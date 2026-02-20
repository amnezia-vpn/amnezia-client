#include "apiCountriesRegionModel.h"

#include <utility>
#include <QSettings>
#include <QVariantMap>

#include "apiCountryModel.h"

ApiCountriesRegionModel::ApiCountriesRegionModel(ApiCountryModel *sourceModel, QObject *parent)
    : QAbstractListModel(parent), m_sourceModel(sourceModel)
{
    m_regionDefinitions = {
        {
            "Europe",
            {
                {"BE", "Belgium", "Бельгия"},
                {"EE", "Estonia", "Эстония"},
                {"FI", "Finland", "Финляндия"},
                {"FR", "France", "Франция"},
                {"GE", "Georgia", "Грузия"},
                {"DE", "Germany", "Германия"},
                {"NL", "Netherlands", "Нидерланды"},
                {"PL", "Poland", "Польша"},
                {"RU", "Russia", "Россия"},
                {"ES", "Spain", "Испания"},
                {"SE", "Sweden", "Швеция"},
                {"CH", "Switzerland", "Швейцария"},
                {"TR", "Turkey", "Турция"},
            },
        },
        {
            "America",
            {
                {"BR", "Brazil", "Бразилия"},
                {"CA", "Canada East", "Канада"},
                {"US", "USA East", "США"},
                {"US", "USA West", "США"},
            },
        },
        {
            "Asia",
            {
                {"AE", "UAE", "ОАЭ"},
                {"JP", "Japan", "Япония"},
                {"KZ", "Kazakhstan", "Казахстан"},
                {"KR", "South Korea", "Южная Корея"},
                {"SG", "Singapore", "Сингапур"},
            },
        },
        {
            "Oceania and Africa",
            {
                {"AU", "Australia", "Австралия"},
                {"NZ", "New Zealand", "Новая Зеландия"},
                {"ZA", "South Africa", "Южная Африка"},
            },
        },
    };

    if (m_sourceModel) {
        connect(m_sourceModel, &QAbstractItemModel::modelReset, this, &ApiCountriesRegionModel::rebuildModel);
        connect(m_sourceModel, &QAbstractItemModel::rowsInserted, this, &ApiCountriesRegionModel::rebuildModel);
        connect(m_sourceModel, &QAbstractItemModel::rowsRemoved, this, &ApiCountriesRegionModel::rebuildModel);
        connect(m_sourceModel, &QAbstractItemModel::dataChanged, this, &ApiCountriesRegionModel::rebuildModel);
    }

    loadRegionExpansionState();
    rebuildModel();
}

int ApiCountriesRegionModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_regions.size();
}

QVariant ApiCountriesRegionModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(rowCount())) {
        return QVariant();
    }

    const RegionData &region = m_regions.at(index.row());
    switch (role) {
    case RegionNameRole:
        return region.regionName;
    case CountriesRole:
        return region.countries;
    default:
        return QVariant();
    }
}

QString ApiCountriesRegionModel::searchText() const
{
    return m_searchText;
}

void ApiCountriesRegionModel::setSearchText(const QString &text)
{
    if (m_searchText == text) {
        return;
    }

    m_searchText = text;
    emit searchTextChanged();
    rebuildModel();
}

bool ApiCountriesRegionModel::isRegionExpanded(const QString &regionName) const
{
    if (isSearchActive()) {
        return true;
    }
    return m_regionsExpanded.contains(regionName) ? m_regionsExpanded.value(regionName) : true;
}

void ApiCountriesRegionModel::toggleRegionExpanded(const QString &regionName)
{
    if (regionName.isEmpty() || isSearchActive()) {
        return;
    }

    const bool currentValue = isRegionExpanded(regionName);
    m_regionsExpanded.insert(regionName, !currentValue);
    saveRegionExpansionState();

    beginResetModel();
    endResetModel();
}

QHash<int, QByteArray> ApiCountriesRegionModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[RegionNameRole] = "regionName";
    roles[CountriesRole] = "countries";
    return roles;
}

QString ApiCountriesRegionModel::normalizeCountryCode(const QString &countryCode) const
{
    return countryCode.trimmed().toUpper();
}

QString ApiCountriesRegionModel::extractCountryIsoCode(const QString &countryCode) const
{
    const QString normalizedCode = normalizeCountryCode(countryCode);

    for (int i = 0; i + 1 < normalizedCode.size(); ++i) {
        const QChar first = normalizedCode.at(i);
        const QChar second = normalizedCode.at(i + 1);
        if (first.isUpper() && second.isUpper()) {
            return normalizedCode.mid(i, 2);
        }
    }

    return normalizedCode;
}

QString ApiCountriesRegionModel::normalizeCountryName(const QString &countryName) const
{
    return countryName.trimmed().toLower();
}

QString ApiCountriesRegionModel::normalizeSearchComparableText(const QString &textValue) const
{
    QString normalizedText = normalizeCountryName(textValue);
    normalizedText.replace(QChar(0x0451), QChar(0x0435)); // ё -> е
    normalizedText.replace(QChar(0x0439), QChar(0x0438)); // й -> и

    QString result;
    result.reserve(normalizedText.size());
    for (int i = 0; i < normalizedText.size(); ++i) {
        const QChar currentChar = normalizedText.at(i);
        const bool isSeparator = currentChar == '.' || currentChar == '-';

        if (!isSeparator) {
            result.append(currentChar);
            continue;
        }

        const QChar prevChar = i > 0 ? normalizedText.at(i - 1) : QChar();
        const QChar nextChar = i + 1 < normalizedText.size() ? normalizedText.at(i + 1) : QChar();
        const bool hasSeparatorNeighbor =
                prevChar == '.' || prevChar == '-' || nextChar == '.' || nextChar == '-';

        if (hasSeparatorNeighbor) {
            result.append(currentChar);
        }
    }

    return result;
}

bool ApiCountriesRegionModel::isCountryMatchingSearch(const QString &countryName, const QString &regionCountryCode,
                                                      const QString &sourceCountryCode, const QString &ruCountryName) const
{
    const QString normalizedSearchText = normalizeSearchComparableText(m_searchText);
    if (normalizedSearchText.isEmpty()) {
        return true;
    }

    const QString normalizedCountryName = normalizeSearchComparableText(countryName);
    const QString normalizedRuCountryName = normalizeSearchComparableText(ruCountryName);
    const QString normalizedRegionCountryCode = normalizeCountryCode(regionCountryCode).toLower();
    const QString normalizedSourceCountryCode = normalizeCountryCode(sourceCountryCode).toLower();

    return normalizedCountryName.startsWith(normalizedSearchText) || normalizedRuCountryName.startsWith(normalizedSearchText) ||
           normalizedRegionCountryCode.startsWith(normalizedSearchText) ||
           normalizedSourceCountryCode.startsWith(normalizedSearchText);
}

bool ApiCountriesRegionModel::isSearchActive() const
{
    return !normalizeSearchComparableText(m_searchText).isEmpty();
}

QString ApiCountriesRegionModel::getDisplayCountryName(const QString &countryName) const
{
    const QString p2pPrefix = "[P2P] ";
    if (countryName.startsWith(p2pPrefix)) {
        return countryName.mid(p2pPrefix.size()) + " [P2P]";
    }
    return countryName;
}

bool ApiCountriesRegionModel::getSourceCountry(int sourceIndex, SourceCountry &country) const
{
    if (!m_sourceModel || sourceIndex < 0 || sourceIndex >= m_sourceModel->rowCount()) {
        return false;
    }

    const QModelIndex modelIndex = m_sourceModel->index(sourceIndex, 0);
    if (!modelIndex.isValid()) {
        return false;
    }

    country.countryName = m_sourceModel->data(modelIndex, ApiCountryModel::CountryNameRole).toString();
    country.countryCode = m_sourceModel->data(modelIndex, ApiCountryModel::CountryCodeRole).toString();
    country.countryImageCode = m_sourceModel->data(modelIndex, ApiCountryModel::CountryImageCodeRole).toString();

    return !country.countryName.isEmpty() && !country.countryCode.isEmpty();
}

int ApiCountriesRegionModel::findCountryIndexByRef(const CountryRef &countryRef, const QHash<int, bool> &usedIndices) const
{
    if (!m_sourceModel) {
        return -1;
    }

    const QString expectedCode = normalizeCountryCode(countryRef.code);
    const QString expectedName = normalizeCountryName(countryRef.name);

    const int countriesCount = m_sourceModel->rowCount();
    for (int i = 0; i < countriesCount; ++i) {
        if (usedIndices.value(i)) {
            continue;
        }

        SourceCountry sourceCountry;
        if (!getSourceCountry(i, sourceCountry)) {
            continue;
        }

        const QString modelCode = normalizeCountryCode(sourceCountry.countryCode);
        const QString modelIsoCode = extractCountryIsoCode(sourceCountry.countryCode);
        const QString modelName = normalizeCountryName(sourceCountry.countryName);

        if (!expectedName.isEmpty() && modelName == expectedName) {
            return i;
        }

        if (!expectedCode.isEmpty() && (modelCode == expectedCode || modelIsoCode == expectedCode)) {
            return i;
        }
    }

    return -1;
}

void ApiCountriesRegionModel::rebuildModel()
{
    beginResetModel();

    QVector<RegionData> regions;
    regions.reserve(m_regionDefinitions.size());
    const auto &regionDefinitions = std::as_const(m_regionDefinitions);
    for (const RegionDefinition &regionDefinition : regionDefinitions) {
        RegionData region;
        region.regionName = regionDefinition.regionName;
        regions.push_back(region);
    }

    QHash<int, bool> usedIndices;
    for (int regionIndex = 0; regionIndex < m_regionDefinitions.size(); ++regionIndex) {
        const RegionDefinition &regionDefinition = m_regionDefinitions.at(regionIndex);
        for (const CountryRef &countryRef : regionDefinition.countries) {
            const int sourceIndex = findCountryIndexByRef(countryRef, usedIndices);

            if (sourceIndex < 0) {
                if (isCountryMatchingSearch(countryRef.name, countryRef.code, countryRef.code, countryRef.ruName)) {
                    QVariantMap fallbackCountry;
                    fallbackCountry.insert("sourceIndex", -1);
                    fallbackCountry.insert("countryName", getDisplayCountryName(countryRef.name));
                    fallbackCountry.insert("sourceCountryName", countryRef.name);
                    fallbackCountry.insert("countryCode", countryRef.code);
                    fallbackCountry.insert("countryImageCode", extractCountryIsoCode(countryRef.code));
                    fallbackCountry.insert("isAvailable", false);
                    regions[regionIndex].countries.push_back(fallbackCountry);
                }
                continue;
            }

            SourceCountry sourceCountry;
            if (!getSourceCountry(sourceIndex, sourceCountry)) {
                continue;
            }

            const QString displayCountryName = getDisplayCountryName(sourceCountry.countryName);
            if (!isCountryMatchingSearch(displayCountryName, countryRef.code, sourceCountry.countryCode, countryRef.ruName)) {
                continue;
            }

            QVariantMap countryData;
            countryData.insert("sourceIndex", sourceIndex);
            countryData.insert("countryName", displayCountryName);
            countryData.insert("sourceCountryName", sourceCountry.countryName);
            countryData.insert("countryCode", sourceCountry.countryCode);
            countryData.insert("countryImageCode", extractCountryIsoCode(sourceCountry.countryImageCode));
            countryData.insert("isAvailable", true);
            regions[regionIndex].countries.push_back(countryData);
            usedIndices.insert(sourceIndex, true);
        }
    }

    m_regions.clear();
    m_regions.reserve(regions.size());
    const auto &regionsConst = std::as_const(regions);
    for (const RegionData &region : regionsConst) {
        if (!region.countries.isEmpty()) {
            m_regions.push_back(region);
        }
    }

    endResetModel();
}

void ApiCountriesRegionModel::loadRegionExpansionState()
{
    QSettings settings;
    const QVariantMap stored = settings.value("PageSettingsApiAvailableCountries/regionsExpanded").toMap();
    m_regionsExpanded.clear();
    for (auto it = stored.constBegin(); it != stored.constEnd(); ++it) {
        m_regionsExpanded.insert(it.key(), it.value().toBool());
    }
}

void ApiCountriesRegionModel::saveRegionExpansionState() const
{
    QVariantMap stored;
    for (auto it = m_regionsExpanded.constBegin(); it != m_regionsExpanded.constEnd(); ++it) {
        stored.insert(it.key(), it.value());
    }

    QSettings settings;
    settings.setValue("PageSettingsApiAvailableCountries/regionsExpanded", stored);
}
