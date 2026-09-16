#include "apiCountryListModel.h"

#include <algorithm>

#include <QCollator>
#include <QLocale>

#include "apiCountryModel.h"
#include "core/repositories/secureAppSettingsRepository.h"
#include "core/utils/api/apiUtils.h"
#include "logger.h"

namespace
{
    Logger logger("ApiCountryListModel");

    constexpr QLatin1String sectionKeySeparator("/");

    constexpr QLatin1String allowlistMarker("[Allowlist]");

    bool isAllowlistName(const QString &countryName)
    {
        return countryName.contains(allowlistMarker, Qt::CaseInsensitive);
    }

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

ApiCountryListModel::ApiCountryListModel(ApiCountryModel *source, SecureAppSettingsRepository *settings,
                                         const QString &listId, QObject *parent)
    : QAbstractListModel(parent), m_source(source), m_settings(settings), m_listId(listId)
{
    m_catalog = countryCatalog::Catalog::bundled();
    if (m_catalog.isEmpty()) {
        logger.error() << "the country catalog is empty, every location will land in the fallback section";
    }

    if (m_settings) {
        m_sortMode = m_settings->countryListSortMode(m_listId) == Alphabetical ? Alphabetical : ByRegion;
    }
    logger.debug() << "list:" << m_listId
                   << "catalog regions:" << m_catalog.regions().size()
                   << "splitThreshold:" << m_catalog.splitThreshold()
                   << "restored sortMode:" << m_sortMode;

    if (m_source) {
        connect(m_source, &ApiCountryModel::modelReset, this, [this]() {
            reloadLocations();
            rebuild();
        });
        connect(m_source, &ApiCountryModel::currentIndexChanged, this, [this]() {
            if (m_rows.isEmpty()) {
                return;
            }
            emit dataChanged(index(0), index(m_rows.size() - 1), { IsCurrentRole });
        });
    }

    reloadLocations();
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
    case IsCurrentRole:
        return m_source && location.sourceIndex == m_source->getCurrentIndex();
    case IsIssuedRole:
    case IsWorkerExpiredRole: {
        if (!m_source) {
            return false;
        }
        const int sourceRole = role == IsIssuedRole ? ApiCountryModel::IsIssuedRole
                                                    : ApiCountryModel::IsWorkerExpiredRole;
        return m_source->data(m_source->index(location.sourceIndex), sourceRole);
    }
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
    if (m_settings) {
        m_settings->setCountryListSortMode(m_listId, m_sortMode);
    }
    emit sortModeChanged();
    rebuild();
}

int ApiCountryListModel::tabFilter() const
{
    return m_tabFilter;
}

void ApiCountryListModel::setTabFilter(int filter)
{
    const int normalized = filter == AllowlistLocations ? AllowlistLocations : AllLocations;
    if (m_tabFilter == normalized) {
        return;
    }
    m_tabFilter = normalized;
    logger.debug() << "tabFilter ->" << (m_tabFilter == AllLocations ? "all" : "allowlist");
    emit tabFilterChanged();
    rebuild();
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
    return m_sortMode == ByRegion;
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
    int headerRow = -1;
    for (int i = 0; i < m_rows.size(); ++i) {
        if (m_rows.at(i).isSectionHeader && m_rows.at(i).sectionKey == sectionKey) {
            headerRow = i;
            break;
        }
    }
    if (headerRow < 0) {
        return;
    }

    const QVector<int> &order = m_sectionOrder[sectionKey];
    if (order.isEmpty()) {
        return;
    }

    m_collapsedSections.insert(sectionKey, collapsed);

    if (collapsed) {
        beginRemoveRows(QModelIndex(), headerRow + 1, headerRow + order.size());
        m_rows.remove(headerRow + 1, order.size());
        endRemoveRows();
    } else {
        beginInsertRows(QModelIndex(), headerRow + 1, headerRow + order.size());
        QVector<Row> inserted;
        inserted.reserve(order.size());
        for (int locationIndex : order) {
            Row row;
            row.locationIndex = locationIndex;
            row.sectionKey = sectionKey;
            inserted.push_back(row);
        }
        m_rows.insert(headerRow + 1, order.size(), Row {});
        std::copy(inserted.cbegin(), inserted.cend(), m_rows.begin() + headerRow + 1);
        endInsertRows();
    }

    notifyCollapsedChanged();
    emit layoutRebuilt();
}

void ApiCountryListModel::expandCurrentSection()
{
    if (!m_source) {
        return;
    }

    const int currentIndex = m_source->getCurrentIndex();
    for (const Location &location : m_locations) {
        if (location.sourceIndex != currentIndex) {
            continue;
        }
        if (m_collapsedSections.value(location.sectionKey, false)) {
            setSectionCollapsed(location.sectionKey, false);
        }
        return;
    }
}

void ApiCountryListModel::clearSearch()
{
    setSearchText({});
}

QString ApiCountryListModel::buildSectionKey(const QString &regionId, const QString &subregionId) const
{
    if (subregionId.isEmpty()) {
        return regionId;
    }
    return regionId + sectionKeySeparator + subregionId;
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
        location.isAllowlist = isAllowlistName(countryInfo.countryName);
        location.countryCode = countryInfo.countryCode;
        location.imageCode = apiUtils::getCountryFlagCode(countryInfo.countryCodeL10n, countryInfo.countryCode);

        const countryCatalog::Entry *entry = m_catalog.find(countryInfo.countryCode, location.imageCode);
        if (entry) {
            location.regionId = entry->regionId;
            location.subregionId = entry->subregionId;
            location.countsTowardSplit = entry->countsTowardSplit;
        } else {
            location.regionId = countryCatalog::otherRegionId;
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

    QHash<QString, int> countableByRegion;
    for (const Location &location : m_locations) {
        if (location.countsTowardSplit) {
            countableByRegion[location.regionId] += 1;
        }
    }

    for (Location &location : m_locations) {
        const bool split = m_catalog.hasSubregions(location.regionId)
                && countableByRegion.value(location.regionId) > m_catalog.splitThreshold();
        if (!split) {
            location.subregionId.clear();
        }
        location.sectionKey = buildSectionKey(location.regionId, location.subregionId);
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
        if (m_tabFilter == AllowlistLocations && !m_locations.at(i).isAllowlist) {
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
            QVector<countryCatalog::Subregion> subregions = region.subregions;
            std::sort(subregions.begin(), subregions.end(),
                      [](const countryCatalog::Subregion &left, const countryCatalog::Subregion &right) {
                          return left.order < right.order;
                      });

            orderedKeys.append(region.id);
            for (const countryCatalog::Subregion &subregion : subregions) {
                orderedKeys.append(buildSectionKey(region.id, subregion.id));
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
        m_sectionCounts.insert(key, candidates.size());

        QVector<int> order;
        order.reserve(candidates.size());
        for (const Candidate &candidate : candidates) {
            order.push_back(candidate.locationIndex);
        }
        m_sectionOrder.insert(key, order);

        if (isGrouped()) {
            Row header;
            header.isSectionHeader = true;
            header.sectionKey = key;
            m_rows.push_back(header);

            if (isSectionCollapsed(key)) {
                continue;
            }
        }

        for (const Candidate &candidate : candidates) {
            Row row;
            row.locationIndex = candidate.locationIndex;
            row.sectionKey = key;
            m_rows.push_back(row);
        }
    }

    endResetModel();

    emit layoutRebuilt();
}
