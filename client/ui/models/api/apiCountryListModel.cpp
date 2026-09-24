#include "apiCountryListModel.h"

#include <algorithm>

#include <QCollator>
#include <QLocale>

#include "apiCountryModel.h"
#include "core/utils/api/apiUtils.h"
#include "logger.h"

namespace
{
    Logger logger("ApiCountryListModel");

    constexpr QLatin1String sectionKeySeparator("/");

    constexpr QLatin1String useCaseAll("all");
    constexpr QLatin1String useCaseFavorites("favorites");
    constexpr QLatin1String useCaseAllowlist("allowlist");
    constexpr QLatin1String useCaseCreated("created");

    constexpr QLatin1String configFilesListId("nativeConfigs");

    constexpr int favoritesLimitValue = 15;

    QString normalizeSpaced(const QString &text)
    {
        QString result;
        result.reserve(text.size());

        for (const QChar &character : text) {
            if (character.isLetterOrNumber()) {
                result.append(character.toLower());
            } else if (character.isSpace() && !result.isEmpty() && !result.endsWith(QChar(' '))) {
                result.append(QChar(' '));
            }
        }

        while (result.endsWith(QChar(' '))) {
            result.chop(1);
        }
        return result;
    }

    QString normalizeTight(const QString &text)
    {
        QString result = normalizeSpaced(text);
        result.remove(QChar(' '));
        return result;
    }

    void appendNormalized(QStringList &spaced, QStringList &tight, const QString &value)
    {
        const QString spacedValue = normalizeSpaced(value);
        if (spacedValue.isEmpty()) {
            return;
        }
        if (!spaced.contains(spacedValue)) {
            spaced.append(spacedValue);
        }
        const QString tightValue = QString(spacedValue).remove(QChar(' '));
        if (!tight.contains(tightValue)) {
            tight.append(tightValue);
        }
    }

    QCollator displayCollator()
    {
        QCollator collator { QLocale() };
        collator.setCaseSensitivity(Qt::CaseInsensitive);
        collator.setNumericMode(true);
        return collator;
    }
}

ApiCountryListModel::ApiCountryListModel(ApiCountryModel *source, const QString &listId, QObject *parent)
    : QAbstractListModel(parent),
      m_source(source),
      m_listId(listId),
      m_useCaseSet(listId == configFilesListId ? UseCaseSet::ConfigFiles : UseCaseSet::Connection),
      m_activeUseCaseId(useCaseAll)
{
    m_catalog = countryCatalog::Catalog::bundled();
    if (m_catalog.isEmpty()) {
        logger.error() << "the country catalog is empty, every location will land in the fallback section";
    }

    logger.debug() << "list:" << m_listId
                   << "catalog regions:" << m_catalog.regions().size()
                   << "splitThreshold:" << m_catalog.splitThreshold();

    if (m_source) {
        connect(m_source, &ApiCountryModel::modelReset, this, [this]() {
            emit sourceAboutToRefresh();
            reloadLocations();
            rebuildUseCases();
            rebuild();
            emit sourceRefreshed();
        });
        connect(m_source, &ApiCountryModel::currentIndexChanged, this, [this]() {
            if (m_rows.isEmpty()) {
                return;
            }
            emit dataChanged(index(0), index(m_rows.size() - 1), { IsCurrentRole });
        });
    }

    reloadLocations();
    rebuildUseCases();
    rebuild();
}

int ApiCountryListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_rows.size();
}

QVariant ApiCountryListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size()) {
        return {};
    }

    const Row &row = m_rows.at(index.row());

    switch (role) {
    case RowTypeRole:
        return row.isSectionHeader ? QStringLiteral("section") : QStringLiteral("country");
    case SectionKeyRole:
        return row.sectionKey;
    }

    if (row.locationIndex < 0 || row.locationIndex >= m_locations.size()) {
        switch (role) {
        case SourceIndexRole:
            return -1;
        case CountryNameRole:
        case SourceCountryNameRole:
        case CountryCodeRole:
        case CountryImageCodeRole:
            return QString();
        case IsCurrentRole:
        case IsIssuedRole:
        case IsWorkerExpiredRole:
        case IsFavoriteRole:
            return false;
        default:
            return {};
        }
    }
    const Location &location = m_locations.at(row.locationIndex);

    switch (role) {
    case SourceIndexRole:
        return location.sourceIndex;
    case CountryNameRole:
        return location.displayName;
    case SourceCountryNameRole:
        return location.sourceName;
    case CountryCodeRole:
        return location.countryCode;
    case CountryImageCodeRole:
        return location.imageCode;
    case IsCurrentRole: {
        if (!m_source) {
            return false;
        }
        const QString currentCountryCode = m_source->getCurrentCountryCode();
        return !currentCountryCode.isEmpty() && location.countryCode == currentCountryCode;
    }
    case IsIssuedRole:
    case IsWorkerExpiredRole: {
        if (!m_source) {
            return false;
        }
        const int sourceRole = role == IsIssuedRole ? ApiCountryModel::IsIssuedRole
                                                    : ApiCountryModel::IsWorkerExpiredRole;
        return m_source->data(m_source->index(location.sourceIndex), sourceRole);
    }
    case IsFavoriteRole:
        return m_favorites.contains(location.countryCode);
    default:
        return {};
    }
}

QHash<int, QByteArray> ApiCountryListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[RowTypeRole] = "rowType";
    roles[SectionKeyRole] = "sectionKey";
    roles[SourceIndexRole] = "sourceIndex";
    roles[CountryNameRole] = "countryName";
    roles[SourceCountryNameRole] = "sourceCountryName";
    roles[CountryCodeRole] = "countryCode";
    roles[CountryImageCodeRole] = "countryImageCode";
    roles[IsCurrentRole] = "isCurrent";
    roles[IsIssuedRole] = "isIssued";
    roles[IsWorkerExpiredRole] = "isWorkerExpired";
    roles[IsFavoriteRole] = "isFavorite";
    return roles;
}

QString ApiCountryListModel::searchText() const
{
    return m_searchText;
}

void ApiCountryListModel::setSearchText(const QString &text)
{
    if (m_searchText == text) {
        return;
    }
    m_searchText = text;
    emit searchTextChanged();
    notifyCollapsedChanged();
    rebuild();
}

int ApiCountryListModel::sortMode() const
{
    return m_sortMode;
}

void ApiCountryListModel::setSortMode(int mode)
{
    const int normalized = mode == Alphabetical ? Alphabetical : ByRegion;
    if (m_sortMode == normalized) {
        return;
    }
    m_sortMode = normalized;
    logger.debug() << "sortMode ->" << (m_sortMode == ByRegion ? "byRegion" : "alphabetical");
    emit sortModeChanged();
    emit groupingChanged();
    rebuild();
}

QString ApiCountryListModel::listId() const
{
    return m_listId;
}

bool ApiCountryListModel::isSearchActive() const
{
    return !normalizeTight(m_searchText).isEmpty();
}

bool ApiCountryListModel::hasResults() const
{
    return !m_rows.isEmpty();
}

bool ApiCountryListModel::isGrouped() const
{
    return m_sortMode == ByRegion && m_activeUseCaseId != useCaseFavorites;
}

QString ApiCountryListModel::activeUseCaseId() const
{
    return m_activeUseCaseId;
}

bool ApiCountryListModel::isIssued(const Location &location) const
{
    if (!m_source || location.sourceIndex < 0) {
        return false;
    }
    return m_source->data(m_source->index(location.sourceIndex), ApiCountryModel::IsIssuedRole).toBool();
}

void ApiCountryListModel::setActiveUseCaseId(const QString &id)
{
    const QString normalized = id.isEmpty() ? QString(useCaseAll) : id;
    if (m_activeUseCaseId == normalized) {
        return;
    }
    bool listed = false;
    for (const QVariant &entry : m_useCases) {
        if (entry.toMap().value(QStringLiteral("useCaseId")).toString() == normalized) {
            listed = true;
            break;
        }
    }
    if (!listed) {
        return;
    }
    m_activeUseCaseId = normalized;
    emit activeUseCaseIdChanged();
    emit groupingChanged();
    rebuildUseCases();
    rebuild();
}

QVariantList ApiCountryListModel::useCases() const
{
    return m_useCases;
}

int ApiCountryListModel::favoritesLimit() const
{
    return favoritesLimitValue;
}

int ApiCountryListModel::catalogVersion() const
{
    return m_catalog.version();
}

QStringList ApiCountryListModel::favorites() const
{
    QStringList codes(m_favorites.cbegin(), m_favorites.cend());
    codes.sort();
    return codes;
}

void ApiCountryListModel::setFavorites(const QStringList &codes)
{
    m_favorites = QSet<QString>(codes.cbegin(), codes.cend());
    m_favorites.remove(QString());
    rebuildUseCases();
    rebuild();
}

QStringList ApiCountryListModel::collapsedSections() const
{
    QStringList keys;
    for (auto it = m_collapsedSections.cbegin(); it != m_collapsedSections.cend(); ++it) {
        if (it.value()) {
            keys.append(it.key());
        }
    }
    keys.sort();
    return keys;
}

void ApiCountryListModel::setCollapsedSections(const QStringList &keys)
{
    m_collapsedSections.clear();
    for (const QString &key : keys) {
        if (!key.isEmpty()) {
            m_collapsedSections.insert(key, true);
        }
    }
    notifyCollapsedChanged();
    rebuild();
}

void ApiCountryListModel::emitCollapsedSections()
{
    emit collapsedSectionsChanged(collapsedSections());
}

const countryCatalog::UseCase *ApiCountryListModel::findUseCase(const QString &id) const
{
    for (const countryCatalog::UseCase &useCase : m_catalog.useCases()) {
        if (useCase.id == id) {
            return &useCase;
        }
    }
    return nullptr;
}

bool ApiCountryListModel::isUseCaseVisible(const countryCatalog::UseCase &useCase) const
{
    if (useCase.id == useCaseAllowlist) {
        return m_source && m_source->getUserCountryCode() == QLatin1String("RU");
    }
    return true;
}

bool ApiCountryListModel::passesActiveUseCase(const Location &location) const
{
    if (m_activeUseCaseId == useCaseAll) {
        return true;
    }
    if (m_activeUseCaseId == useCaseFavorites) {
        return m_favoritesSnapshot.contains(location.countryCode);
    }
    if (m_activeUseCaseId == useCaseCreated) {
        return isIssued(location);
    }
    const countryCatalog::UseCase *useCase = findUseCase(m_activeUseCaseId);
    if (!useCase) {
        return false;
    }
    for (const QString &id : useCase->locationIds) {
        if (id.compare(location.countryCode, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }
    return false;
}

bool ApiCountryListModel::rebuildUseCases()
{
    auto makeEntry = [](const QString &id, int count) {
        QVariantMap entry;
        entry.insert(QStringLiteral("useCaseId"), id);
        entry.insert(QStringLiteral("count"), count);
        return QVariant(entry);
    };

    QVariantList list;

    int favoriteCount = 0;
    for (const Location &location : m_locations) {
        if (m_favorites.contains(location.countryCode)) {
            ++favoriteCount;
        }
    }
    if (favoriteCount > 0) {
        list.append(makeEntry(useCaseFavorites, favoriteCount));
    }

    list.append(makeEntry(useCaseAll, m_locations.size()));

    if (m_useCaseSet == UseCaseSet::ConfigFiles) {
        int issuedCount = 0;
        for (const Location &location : m_locations) {
            if (isIssued(location)) {
                ++issuedCount;
            }
        }
        if (issuedCount > 0) {
            list.append(makeEntry(useCaseCreated, issuedCount));
        }
    }

    QVector<countryCatalog::UseCase> catalogUseCases;
    if (m_useCaseSet == UseCaseSet::Connection) {
        catalogUseCases = m_catalog.useCases();
    }
    std::sort(catalogUseCases.begin(), catalogUseCases.end(),
              [](const countryCatalog::UseCase &left, const countryCatalog::UseCase &right) {
                  return left.order < right.order;
              });
    for (const countryCatalog::UseCase &useCase : catalogUseCases) {
        if (!isUseCaseVisible(useCase)) {
            continue;
        }
        int count = 0;
        for (const Location &location : m_locations) {
            for (const QString &id : useCase.locationIds) {
                if (id.compare(location.countryCode, Qt::CaseInsensitive) == 0) {
                    ++count;
                    break;
                }
            }
        }
        if (count > 0) {
            list.append(makeEntry(useCase.id, count));
        }
    }

    if (list != m_useCases) {
        m_useCases = list;
        emit useCasesChanged();
    }

    bool activeStillListed = false;
    for (const QVariant &entry : m_useCases) {
        if (entry.toMap().value(QStringLiteral("useCaseId")).toString() == m_activeUseCaseId) {
            activeStillListed = true;
            break;
        }
    }
    if (activeStillListed) {
        return false;
    }
    m_activeUseCaseId = useCaseAll;
    emit activeUseCaseIdChanged();
    emit groupingChanged();
    return true;
}

int ApiCountryListModel::collapsedRevision() const
{
    return m_collapsedRevision;
}

void ApiCountryListModel::notifyCollapsedChanged()
{
    m_collapsedRevision += 1;
    emit collapsedRevisionChanged();
}

QString ApiCountryListModel::sectionRegionId(const QString &sectionKey) const
{
    return sectionKey.section(sectionKeySeparator, 0, 0);
}

QString ApiCountryListModel::sectionSubregionId(const QString &sectionKey) const
{
    return sectionKey.section(sectionKeySeparator, 1, 1);
}

QString ApiCountryListModel::sectionSubsubregionId(const QString &sectionKey) const
{
    return sectionKey.section(sectionKeySeparator, 2, 2);
}

int ApiCountryListModel::sectionCount(const QString &sectionKey) const
{
    return m_sectionCounts.value(sectionKey, 0);
}

QString ApiCountryListModel::sectionKeyAtRow(int row) const
{
    if (row < 0 || row >= m_rows.size()) {
        return {};
    }
    return m_rows.at(row).sectionKey;
}

bool ApiCountryListModel::isSectionCollapsed(const QString &sectionKey) const
{
    if (isSearchActive()) {
        return false;
    }
    return m_collapsedSections.value(sectionKey, false);
}

void ApiCountryListModel::toggleSection(const QString &sectionKey)
{
    if (sectionKey.isEmpty() || isSearchActive()) {
        return;
    }
    setSectionCollapsed(sectionKey, !m_collapsedSections.value(sectionKey, false));
}

void ApiCountryListModel::setSectionCollapsed(const QString &sectionKey, bool collapsed)
{
    if (m_sectionCounts.value(sectionKey) == 0) {
        return;
    }

    m_collapsedSections.insert(sectionKey, collapsed);

    applyRows(buildRows());

    notifyCollapsedChanged();
    emitCollapsedSections();
}

void ApiCountryListModel::applyRows(const QVector<Row> &next)
{
    const int oldSize = m_rows.size();
    const int newSize = next.size();
    const int shorter = std::min(oldSize, newSize);

    int prefix = 0;
    while (prefix < shorter && m_rows.at(prefix) == next.at(prefix)) {
        ++prefix;
    }
    int suffix = 0;
    while (suffix < shorter - prefix
           && m_rows.at(oldSize - 1 - suffix) == next.at(newSize - 1 - suffix)) {
        ++suffix;
    }

    if (newSize < oldSize) {
        beginRemoveRows(QModelIndex(), prefix, oldSize - suffix - 1);
        m_rows = next;
        endRemoveRows();
    } else if (newSize > oldSize) {
        beginInsertRows(QModelIndex(), prefix, newSize - suffix - 1);
        m_rows = next;
        endInsertRows();
    } else {
        m_rows = next;
    }

    m_layoutRevision += 1;
    emit layoutRebuilt();
}

void ApiCountryListModel::expandCurrentSection()
{
    if (!m_source) {
        return;
    }

    const QString currentCountryCode = m_source->getCurrentCountryCode();
    if (currentCountryCode.isEmpty()) {
        return;
    }

    for (const Location &location : m_locations) {
        if (location.countryCode != currentCountryCode) {
            continue;
        }

        QStringList chain { location.sectionKey };
        for (QString key = parentSectionKey(location.sectionKey); !key.isEmpty(); key = parentSectionKey(key)) {
            chain.prepend(key);
        }
        bool changedFlat = false;
        for (const QString &key : chain) {
            if (key.isEmpty() || !m_collapsedSections.value(key, false)) {
                continue;
            }
            if (isGrouped()) {
                setSectionCollapsed(key, false);
            } else {
                m_collapsedSections.insert(key, false);
                changedFlat = true;
            }
        }
        if (changedFlat) {
            notifyCollapsedChanged();
            emitCollapsedSections();
        }
        return;
    }
}

void ApiCountryListModel::clearSearch()
{
    setSearchText({});
}

bool ApiCountryListModel::toggleFavorite(const QString &countryCode)
{
    if (countryCode.isEmpty()) {
        return false;
    }

    if (m_favorites.contains(countryCode)) {
        m_favorites.remove(countryCode);
    } else {
        int visibleFavorites = 0;
        for (const Location &location : m_locations) {
            if (m_favorites.contains(location.countryCode)) {
                ++visibleFavorites;
            }
        }
        if (visibleFavorites >= favoritesLimitValue) {
            emit favoritesLimitExceeded();
            return false;
        }
        m_favorites.insert(countryCode);
    }

    for (int i = 0; i < m_rows.size(); ++i) {
        const Row &row = m_rows.at(i);
        if (row.isSectionHeader || row.locationIndex < 0 || row.locationIndex >= m_locations.size()) {
            continue;
        }
        if (m_locations.at(row.locationIndex).countryCode == countryCode) {
            emit dataChanged(index(i), index(i), { IsFavoriteRole });
        }
    }

    emit favoritesChanged(favorites());

    if (rebuildUseCases()) {
        rebuild();
    }
    return true;
}

int ApiCountryListModel::layoutRevision() const
{
    return m_layoutRevision;
}

bool ApiCountryListModel::isSectionHeaderRow(int row) const
{
    return row >= 0 && row < m_rows.size() && m_rows.at(row).isSectionHeader;
}

int ApiCountryListModel::rowForSectionHeader(const QString &sectionKey) const
{
    for (int i = 0; i < m_rows.size(); ++i) {
        if (m_rows.at(i).isSectionHeader && m_rows.at(i).sectionKey == sectionKey) {
            return i;
        }
    }
    return -1;
}

int ApiCountryListModel::rowForCountryCode(const QString &countryCode) const
{
    if (countryCode.isEmpty()) {
        return -1;
    }
    for (int i = 0; i < m_rows.size(); ++i) {
        const Row &row = m_rows.at(i);
        if (row.isSectionHeader || row.locationIndex < 0 || row.locationIndex >= m_locations.size()) {
            continue;
        }
        if (m_locations.at(row.locationIndex).countryCode == countryCode) {
            return i;
        }
    }
    return -1;
}

void ApiCountryListModel::applyDefaultState(bool followCurrentLocation)
{
    clearSearch();

    const QString current = (followCurrentLocation && m_source) ? m_source->getCurrentCountryCode()
                                                                : QString();

    bool hasFavoritesChip = false;
    for (const QVariant &entry : m_useCases) {
        if (entry.toMap().value(QStringLiteral("useCaseId")).toString() == useCaseFavorites) {
            hasFavoritesChip = true;
            break;
        }
    }

    QString target = useCaseAll;
    if (hasFavoritesChip) {
        if (!followCurrentLocation) {
            target = useCaseFavorites;
        } else if (!current.isEmpty() && m_favorites.contains(current)) {
            target = useCaseFavorites;
        }
    }

    if (m_activeUseCaseId != target) {
        setActiveUseCaseId(target);
    } else {
        rebuild();
    }

    if (!followCurrentLocation || current.isEmpty()) {
        return;
    }
    expandCurrentSection();
    emit positionRequested(rowForCountryCode(current));
}

QString ApiCountryListModel::buildSectionKey(const QString &regionId, const QString &subregionId,
                                             const QString &subsubregionId) const
{
    if (subregionId.isEmpty()) {
        return regionId;
    }
    if (subsubregionId.isEmpty()) {
        return regionId + sectionKeySeparator + subregionId;
    }
    return regionId + sectionKeySeparator + subregionId + sectionKeySeparator + subsubregionId;
}

QString ApiCountryListModel::parentSectionKey(const QString &sectionKey) const
{
    const int separator = sectionKey.lastIndexOf(sectionKeySeparator);
    if (separator < 0) {
        return {};
    }
    return sectionKey.left(separator);
}

bool ApiCountryListModel::isHiddenByParent(const QString &sectionKey) const
{
    for (QString parent = parentSectionKey(sectionKey); !parent.isEmpty(); parent = parentSectionKey(parent)) {
        if (isSectionCollapsed(parent)) {
            return true;
        }
    }
    return false;
}

void ApiCountryListModel::reloadLocations()
{
    m_locations.clear();
    if (!m_source) {
        return;
    }

    const QVector<ApiCountryModel::CountryInfo> &countries = m_source->countries();
    m_locations.reserve(countries.size());

    for (int i = 0; i < countries.size(); ++i) {
        const ApiCountryModel::CountryInfo &countryInfo = countries.at(i);

        Location location;
        location.sourceIndex = i;
        location.sourceName = countryInfo.countryName;
        location.displayName = countryInfo.countryName;
        location.countryCode = countryInfo.countryCode;
        location.imageCode = apiUtils::getCountryFlagCode(countryInfo.countryCodeL10n, countryInfo.countryCode);

        const countryCatalog::Entry *entry = m_catalog.find(countryInfo.countryCode, location.imageCode);
        if (entry) {
            location.regionId = entry->regionId;
            location.subregionId = entry->subregionId;
            location.subsubregionId = entry->subsubregionId;
        } else {
            location.regionId = countryCatalog::otherRegionId;
        }

        const countryCatalog::Region *region = nullptr;
        for (const countryCatalog::Region &candidate : m_catalog.regions()) {
            if (candidate.id == location.regionId) {
                region = &candidate;
                break;
            }
        }
        if (!region) {
            location.regionId = countryCatalog::otherRegionId;
            location.subregionId.clear();
            location.subsubregionId.clear();
        } else if (!location.subregionId.isEmpty()) {
            const countryCatalog::Subregion *subregion = nullptr;
            for (const countryCatalog::Subregion &candidate : region->subregions) {
                if (candidate.id == location.subregionId) {
                    subregion = &candidate;
                    break;
                }
            }
            if (!subregion) {
                location.subregionId.clear();
                location.subsubregionId.clear();
            } else if (!location.subsubregionId.isEmpty()) {
                bool known = false;
                for (const countryCatalog::Subsubregion &subsubregion : subregion->subsubregions) {
                    if (subsubregion.id == location.subsubregionId) {
                        known = true;
                        break;
                    }
                }
                if (!known) {
                    location.subsubregionId.clear();
                }
            }
        } else {
            location.subsubregionId.clear();
        }

        QStringList exact;
        appendNormalized(location.search.spaced, location.search.tight, location.displayName);
        appendNormalized(location.search.spaced, location.search.tight, location.sourceName);
        if (entry) {
            appendNormalized(location.search.spaced, location.search.tight, entry->nameEn);
            appendNormalized(location.search.spaced, location.search.tight, entry->nameRu);
            appendNormalized(location.search.spaced, location.search.tight, entry->city);

            for (const QString &alias : entry->aliases) {
                appendNormalized(location.search.spaced, location.search.tight, alias);
                const QString tightAlias = normalizeTight(alias);
                if (!tightAlias.isEmpty() && !exact.contains(tightAlias)) {
                    exact.append(tightAlias);
                }
            }
        }
        const QString tightIso = normalizeTight(location.imageCode);
        if (!tightIso.isEmpty() && !exact.contains(tightIso)) {
            exact.append(tightIso);
        }
        location.search.exact = exact;

        m_locations.push_back(location);
    }

    QHash<QString, int> visibleCounts;
    for (const Location &location : m_locations) {
        visibleCounts[location.regionId] += 1;
        if (!location.subregionId.isEmpty()) {
            visibleCounts[buildSectionKey(location.regionId, location.subregionId)] += 1;
        }
    }

    for (Location &location : m_locations) {
        if (!m_catalog.isSplit(location.regionId, visibleCounts.value(location.regionId))) {
            location.subregionId.clear();
        }
        if (location.subregionId.isEmpty()
            || !m_catalog.isSplit(location.regionId, location.subregionId,
                                  visibleCounts.value(buildSectionKey(location.regionId, location.subregionId)))) {
            location.subsubregionId.clear();
        }
        location.sectionKey = buildSectionKey(location.regionId, location.subregionId, location.subsubregionId);
    }


    QStringList unmatched;
    for (const Location &location : m_locations) {
        if (location.regionId == countryCatalog::otherRegionId) {
            unmatched.append(location.countryCode);
        }
    }
    if (!unmatched.isEmpty()) {
        logger.warning() << "not in the catalog, fell back to" << countryCatalog::otherRegionId
                         << ":" << unmatched.join(QStringLiteral(", "));
    }
}

int ApiCountryListModel::matchLevel(const Location &location, const QString &spacedQuery,
                                    const QString &tightQuery) const
{
    if (tightQuery.isEmpty()) {
        return 1;
    }

    for (const QString &value : location.search.exact) {
        if (value == tightQuery) {
            return 1;
        }
    }

    for (const QString &value : location.search.spaced) {
        if (value.startsWith(spacedQuery)) {
            return 2;
        }
        const QStringList words = value.split(QChar(' '), Qt::SkipEmptyParts);
        for (const QString &word : words) {
            if (word.startsWith(tightQuery)) {
                return 2;
            }
        }
    }

    for (const QString &value : location.search.tight) {
        if (value.startsWith(tightQuery)) {
            return 2;
        }
    }

    for (const QString &value : location.search.spaced) {
        if (value.contains(spacedQuery)) {
            return 3;
        }
    }

    return 0;
}

void ApiCountryListModel::rebuild()
{
    beginResetModel();

    m_favoritesSnapshot = m_favorites;

    m_rows.clear();
    m_sectionCounts.clear();
    m_sectionOrder.clear();

    const QString spacedQuery = normalizeSpaced(m_searchText);
    const QString tightQuery = normalizeTight(m_searchText);

    struct Candidate
    {
        int locationIndex;
        int level;
    };
    QHash<QString, QVector<Candidate>> grouped;

    for (int i = 0; i < m_locations.size(); ++i) {
        if (!passesActiveUseCase(m_locations.at(i))) {
            continue;
        }
        const int level = matchLevel(m_locations.at(i), spacedQuery, tightQuery);
        if (level == 0) {
            continue;
        }
        const QString key = isGrouped() ? m_locations.at(i).sectionKey : QString();
        grouped[key].push_back({ i, level });
    }

    const QCollator collator = displayCollator();
    auto byLevelThenName = [this, &collator](const Candidate &left, const Candidate &right) {
        if (left.level != right.level) {
            return left.level < right.level;
        }
        return collator.compare(m_locations.at(left.locationIndex).displayName,
                                m_locations.at(right.locationIndex).displayName) < 0;
    };

    QStringList orderedKeys;
    if (isGrouped()) {
        QVector<countryCatalog::Region> regions = m_catalog.regions();
        std::sort(regions.begin(), regions.end(),
                  [](const countryCatalog::Region &left, const countryCatalog::Region &right) {
                      return left.order < right.order;
                  });

        for (const countryCatalog::Region &region : regions) {
            if (region.id == countryCatalog::otherRegionId || orderedKeys.contains(region.id)) {
                continue;
            }
            QVector<countryCatalog::Subregion> subregions = region.subregions;
            std::sort(subregions.begin(), subregions.end(),
                      [](const countryCatalog::Subregion &left, const countryCatalog::Subregion &right) {
                          return left.order < right.order;
                      });

            orderedKeys.append(region.id);
            for (const countryCatalog::Subregion &subregion : subregions) {
                orderedKeys.append(buildSectionKey(region.id, subregion.id));

                QVector<countryCatalog::Subsubregion> subsubregions = subregion.subsubregions;
                std::sort(subsubregions.begin(), subsubregions.end(),
                          [](const countryCatalog::Subsubregion &left, const countryCatalog::Subsubregion &right) {
                              return left.order < right.order;
                          });
                for (const countryCatalog::Subsubregion &subsubregion : subsubregions) {
                    orderedKeys.append(buildSectionKey(region.id, subregion.id, subsubregion.id));
                }
            }
        }
        orderedKeys.append(countryCatalog::otherRegionId);
    } else {
        orderedKeys.append(QString());
    }

    for (const QString &key : orderedKeys) {
        auto candidates = grouped.value(key);
        if (candidates.isEmpty()) {
            continue;
        }

        std::sort(candidates.begin(), candidates.end(), byLevelThenName);
        m_sectionCounts[key] += candidates.size();
        for (QString parent = parentSectionKey(key); !parent.isEmpty(); parent = parentSectionKey(parent)) {
            m_sectionCounts[parent] += candidates.size();
        }

        QVector<int> order;
        order.reserve(candidates.size());
        for (const Candidate &candidate : candidates) {
            order.push_back(candidate.locationIndex);
        }
        m_sectionOrder.insert(key, order);
    }

    m_orderedSectionKeys.clear();
    for (const QString &key : orderedKeys) {
        if (m_sectionCounts.value(key) > 0) {
            m_orderedSectionKeys.append(key);
        }
    }

    int matched = 0;
    for (auto it = grouped.cbegin(); it != grouped.cend(); ++it) {
        matched += it.value().size();
    }
    int placed = 0;
    for (const QString &key : m_orderedSectionKeys) {
        if (parentSectionKey(key).isEmpty()) {
            placed += m_sectionCounts.value(key);
        }
    }
    if (placed != matched) {
        logger.error() << "list:" << m_listId << matched << "locations matched but" << placed
                       << "were placed into sections";
    }

    m_rows = buildRows();
    m_layoutRevision += 1;

    endResetModel();

    emit layoutRebuilt();
}

QVector<ApiCountryListModel::Row> ApiCountryListModel::buildRows() const
{
    QVector<Row> rows;

    for (const QString &key : m_orderedSectionKeys) {
        if (isGrouped()) {
            if (isHiddenByParent(key)) {
                continue;
            }

            Row header;
            header.isSectionHeader = true;
            header.sectionKey = key;
            rows.push_back(header);

            if (isSectionCollapsed(key)) {
                continue;
            }
        }

        for (int locationIndex : m_sectionOrder.value(key)) {
            Row row;
            row.locationIndex = locationIndex;
            row.sectionKey = key;
            rows.push_back(row);
        }
    }

    return rows;
}
