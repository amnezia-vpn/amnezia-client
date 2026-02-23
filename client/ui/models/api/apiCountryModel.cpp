#include "apiCountryModel.h"

#include <QJsonObject>
#include <QSettings>
#include <utility>

#include "core/api/apiDefs.h"
#include "logger.h"

namespace
{
    Logger logger("ApiCountryModel");
    constexpr QLatin1String countryConfig("country_config");

    struct RegionRowData
    {
        bool isRegionHeader = false;
        QString regionName;
        bool isExpanded = true;
        int sourceIndex = -1;
        QString countryName;
        QString sourceCountryName;
        QString countryCode;
        QString countryImageCode;
    };
}

class ApiCountryModel::RegionRowsModel : public QAbstractListModel
{
public:
    enum Roles {
        RowTypeRole = Qt::UserRole + 1,
        RegionNameRole,
        IsExpandedRole,
        SourceIndexRole,
        CountryNameRole,
        SourceCountryNameRole,
        CountryCodeRole,
        CountryImageCodeRole
    };

    explicit RegionRowsModel(QObject *parent = nullptr) : QAbstractListModel(parent) {}

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        Q_UNUSED(parent)
        return m_rows.size();
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size()) {
            return QVariant();
        }

        const RegionRowData &row = m_rows.at(index.row());
        switch (role) {
        case RowTypeRole:
            return row.isRegionHeader ? "region" : "country";
        case RegionNameRole:
            return row.regionName;
        case IsExpandedRole:
            return row.isExpanded;
        case SourceIndexRole:
            return row.sourceIndex;
        case CountryNameRole:
            return row.countryName;
        case SourceCountryNameRole:
            return row.sourceCountryName;
        case CountryCodeRole:
            return row.countryCode;
        case CountryImageCodeRole:
            return row.countryImageCode;
        default:
            return QVariant();
        }
    }

    QHash<int, QByteArray> roleNames() const override
    {
        QHash<int, QByteArray> roles;
        roles[RowTypeRole] = "rowType";
        roles[RegionNameRole] = "regionName";
        roles[IsExpandedRole] = "isExpanded";
        roles[SourceIndexRole] = "sourceIndex";
        roles[CountryNameRole] = "countryName";
        roles[SourceCountryNameRole] = "sourceCountryName";
        roles[CountryCodeRole] = "countryCode";
        roles[CountryImageCodeRole] = "countryImageCode";
        return roles;
    }

    void setRows(QVector<RegionRowData> &&rows)
    {
        beginResetModel();
        m_rows = std::move(rows);
        endResetModel();
    }

private:
    QVector<RegionRowData> m_rows;
};

ApiCountryModel::ApiCountryModel(QObject *parent)
    : QAbstractListModel(parent), m_regionRowsModel(std::make_unique<RegionRowsModel>(this))
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

    loadRegionExpansionState();
    rebuildGroupedRegions();
}

ApiCountryModel::~ApiCountryModel() = default;

int ApiCountryModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_countries.size();
}

QVariant ApiCountryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(rowCount())) {
        return QVariant();
    }

    const CountryInfo &countryInfo = m_countries.at(index.row());
    const IssuedConfigInfo issuedConfigInfo = m_issuedConfigs.value(countryInfo.countryCode);
    const bool isIssued = issuedConfigInfo.sourceType == countryConfig;

    switch (role) {
    case CountryCodeRole:
        return countryInfo.countryCode;
    case CountryNameRole:
        return countryInfo.countryName;
    case CountryImageCodeRole:
        return countryInfo.countryCode.toUpper();
    case IsIssuedRole:
        return isIssued;
    case IsWorkerExpiredRole:
        return issuedConfigInfo.lastDownloaded < issuedConfigInfo.workerLastUpdated;
    default:
        return QVariant();
    }
}

void ApiCountryModel::updateModel(const QJsonArray &countries, const QString &currentCountryCode)
{
    beginResetModel();

    m_countries.clear();
    for (int i = 0; i < countries.size(); ++i) {
        CountryInfo countryInfo;
        const QJsonObject countryObject = countries.at(i).toObject();

        countryInfo.countryName = countryObject.value(apiDefs::key::serverCountryName).toString();
        countryInfo.countryCode = countryObject.value(apiDefs::key::serverCountryCode).toString();

        if (countryInfo.countryCode == currentCountryCode) {
            m_currentIndex = i;
            emit currentIndexChanged(m_currentIndex);
        }
        m_countries.push_back(countryInfo);
    }

    endResetModel();
    rebuildGroupedRegions();
}

void ApiCountryModel::updateIssuedConfigsInfo(const QJsonArray &issuedConfigs)
{
    beginResetModel();

    m_issuedConfigs.clear();
    for (int i = 0; i < issuedConfigs.size(); ++i) {
        IssuedConfigInfo issuedConfigInfo;
        const QJsonObject issuedConfigObject = issuedConfigs.at(i).toObject();

        if (issuedConfigObject.value(apiDefs::key::sourceType).toString() != countryConfig) {
            continue;
        }

        issuedConfigInfo.installationUuid = issuedConfigObject.value(apiDefs::key::installationUuid).toString();
        issuedConfigInfo.workerLastUpdated = issuedConfigObject.value(apiDefs::key::workerLastUpdated).toString();
        issuedConfigInfo.lastDownloaded = issuedConfigObject.value(apiDefs::key::lastDownloaded).toString();
        issuedConfigInfo.sourceType = issuedConfigObject.value(apiDefs::key::sourceType).toString();
        issuedConfigInfo.osVersion = issuedConfigObject.value(apiDefs::key::osVersion).toString();

        m_issuedConfigs.insert(issuedConfigObject.value(apiDefs::key::serverCountryCode).toString(), issuedConfigInfo);
    }

    endResetModel();
}

int ApiCountryModel::getCurrentIndex()
{
    return m_currentIndex;
}

void ApiCountryModel::setCurrentIndex(const int i)
{
    m_currentIndex = i;
    emit currentIndexChanged(m_currentIndex);
}

QString ApiCountryModel::searchText() const
{
    return m_searchText;
}

void ApiCountryModel::setSearchText(const QString &text)
{
    if (m_searchText == text) {
        return;
    }

    m_searchText = text;
    emit searchTextChanged();
    rebuildGroupedRegions();
}

QAbstractListModel *ApiCountryModel::regionRowsModel() const
{
    return m_regionRowsModel.get();
}

bool ApiCountryModel::hasVisibleRegions() const
{
    return m_regionRowsModel && m_regionRowsModel->rowCount() > 0;
}

bool ApiCountryModel::isRegionExpanded(const QString &regionName) const
{
    if (isSearchActive()) {
        return true;
    }
    return m_regionsExpanded.contains(regionName) ? m_regionsExpanded.value(regionName) : true;
}

void ApiCountryModel::toggleRegionExpanded(const QString &regionName)
{
    if (regionName.isEmpty() || isSearchActive()) {
        return;
    }

    const bool currentValue = isRegionExpanded(regionName);
    m_regionsExpanded.insert(regionName, !currentValue);
    saveRegionExpansionState();
    rebuildGroupedRegions();
}

QHash<int, QByteArray> ApiCountryModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[CountryNameRole] = "countryName";
    roles[CountryCodeRole] = "countryCode";
    roles[CountryImageCodeRole] = "countryImageCode";
    roles[IsIssuedRole] = "isIssued";
    roles[IsWorkerExpiredRole] = "isWorkerExpired";
    return roles;
}

QString ApiCountryModel::normalizeCountryCode(const QString &countryCode) const
{
    return countryCode.trimmed().toUpper();
}

QString ApiCountryModel::extractCountryIsoCode(const QString &countryCode) const
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

QString ApiCountryModel::normalizeCountryName(const QString &countryName) const
{
    return countryName.trimmed().toLower();
}

QString ApiCountryModel::normalizeSearchComparableText(const QString &textValue) const
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
        const bool hasSeparatorNeighbor = prevChar == '.' || prevChar == '-' || nextChar == '.' || nextChar == '-';
        if (hasSeparatorNeighbor) {
            result.append(currentChar);
        }
    }

    return result;
}

bool ApiCountryModel::isCountryMatchingSearch(const QString &countryName, const QString &regionCountryCode,
                                              const QString &sourceCountryCode, const QString &ruCountryName,
                                              const QString &normalizedSearchText) const
{
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

QString ApiCountryModel::getDisplayCountryName(const QString &countryName) const
{
    const QString p2pPrefix = "[P2P] ";
    if (countryName.startsWith(p2pPrefix)) {
        return countryName.mid(p2pPrefix.size()) + " [P2P]";
    }
    return countryName;
}

int ApiCountryModel::findCountryIndexByRef(const CountryRef &countryRef, const QHash<int, bool> &usedIndices) const
{
    const QString expectedCode = normalizeCountryCode(countryRef.code);
    const QString expectedName = normalizeCountryName(countryRef.name);
    const int countriesCount = m_countries.size();

    for (int i = 0; i < countriesCount; ++i) {
        if (usedIndices.value(i)) {
            continue;
        }

        const CountryInfo &country = m_countries.at(i);
        const QString modelCode = normalizeCountryCode(country.countryCode);
        const QString modelIsoCode = extractCountryIsoCode(country.countryCode);
        const QString modelName = normalizeCountryName(country.countryName);

        if (!expectedName.isEmpty() && modelName == expectedName) {
            return i;
        }
        if (!expectedCode.isEmpty() && (modelCode == expectedCode || modelIsoCode == expectedCode)) {
            return i;
        }
    }

    return -1;
}

void ApiCountryModel::rebuildGroupedRegions()
{
    QVector<RegionRowData> rows;
    QHash<int, bool> usedIndices;
    const QString normalizedSearchText = normalizeSearchComparableText(m_searchText);

    for (int regionIndex = 0; regionIndex < m_regionDefinitions.size(); ++regionIndex) {
        const RegionDefinition &regionDefinition = m_regionDefinitions.at(regionIndex);
        QVector<RegionRowData> countries;

        const auto &countryRefs = std::as_const(regionDefinition.countries);
        for (const CountryRef &countryRef : countryRefs) {
            const int sourceIndex = findCountryIndexByRef(countryRef, usedIndices);
            if (sourceIndex < 0) {
                continue;
            }

            const CountryInfo &sourceCountry = m_countries.at(sourceIndex);
            const QString displayCountryName = getDisplayCountryName(sourceCountry.countryName);
            if (!isCountryMatchingSearch(displayCountryName, countryRef.code, sourceCountry.countryCode, countryRef.ruName,
                                         normalizedSearchText)) {
                continue;
            }

            RegionRowData countryRow;
            countryRow.isRegionHeader = false;
            countryRow.regionName = regionDefinition.regionName;
            countryRow.sourceIndex = sourceIndex;
            countryRow.countryName = displayCountryName;
            countryRow.sourceCountryName = sourceCountry.countryName;
            countryRow.countryCode = sourceCountry.countryCode;
            countryRow.countryImageCode = extractCountryIsoCode(sourceCountry.countryCode);
            countries.push_back(std::move(countryRow));
            usedIndices.insert(sourceIndex, true);
        }

        if (!countries.isEmpty()) {
            const bool expanded = isRegionExpanded(regionDefinition.regionName);

            RegionRowData headerRow;
            headerRow.isRegionHeader = true;
            headerRow.regionName = regionDefinition.regionName;
            headerRow.isExpanded = expanded;
            rows.push_back(std::move(headerRow));

            if (expanded) {
                for (RegionRowData &countryRow : countries) {
                    countryRow.isExpanded = expanded;
                    rows.push_back(std::move(countryRow));
                }
            }
        }
    }

    m_regionRowsModel->setRows(std::move(rows));
    emit regionRowsChanged();
}

void ApiCountryModel::loadRegionExpansionState()
{
    QSettings settings;
    const QVariantMap stored = settings.value("PageSettingsApiAvailableCountries/regionsExpanded").toMap();
    m_regionsExpanded.clear();
    for (auto it = stored.constBegin(); it != stored.constEnd(); ++it) {
        m_regionsExpanded.insert(it.key(), it.value().toBool());
    }
}

void ApiCountryModel::saveRegionExpansionState() const
{
    QVariantMap stored;
    for (auto it = m_regionsExpanded.constBegin(); it != m_regionsExpanded.constEnd(); ++it) {
        stored.insert(it.key(), it.value());
    }

    QSettings settings;
    settings.setValue("PageSettingsApiAvailableCountries/regionsExpanded", stored);
}

bool ApiCountryModel::isSearchActive() const
{
    return !normalizeSearchComparableText(m_searchText).isEmpty();
}
