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
    constexpr QLatin1String regionEurope("Europe");
    constexpr QLatin1String regionAmerica("America");
    constexpr QLatin1String regionAsia("Asia");
    constexpr QLatin1String regionOceaniaAfrica("Oceania and Africa");
    constexpr QLatin1String regionOther("Other");

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

    QString resolveRegionByIsoCode(const QString &isoCode)
    {
        static const QHash<QString, QString> isoToRegion = {
            {"BE", regionEurope},
            {"EE", regionEurope},
            {"FI", regionEurope},
            {"FR", regionEurope},
            {"GE", regionEurope},
            {"DE", regionEurope},
            {"NL", regionEurope},
            {"PL", regionEurope},
            {"RU", regionEurope},
            {"ES", regionEurope},
            {"SE", regionEurope},
            {"CH", regionEurope},
            {"TR", regionEurope},
            {"BR", regionAmerica},
            {"CA", regionAmerica},
            {"US", regionAmerica},
            {"AE", regionAsia},
            {"JP", regionAsia},
            {"KZ", regionAsia},
            {"KR", regionAsia},
            {"SG", regionAsia},
            {"AU", regionOceaniaAfrica},
            {"NZ", regionOceaniaAfrica},
            {"ZA", regionOceaniaAfrica},
        };

        return isoToRegion.value(isoCode, regionOther);
    }
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

bool ApiCountryModel::isCountryMatchingSearch(const QString &countryName, const QString &sourceCountryCode,
                                              const QString &normalizedSearchText) const
{
    if (normalizedSearchText.isEmpty()) {
        return true;
    }

    const QString normalizedCountryName = normalizeSearchComparableText(countryName);
    const QString normalizedSourceCountryCode = normalizeCountryCode(sourceCountryCode).toLower();

    return normalizedCountryName.startsWith(normalizedSearchText) || normalizedSourceCountryCode.startsWith(normalizedSearchText);
}

QString ApiCountryModel::getDisplayCountryName(const QString &countryName) const
{
    const QString p2pPrefix = "[P2P] ";
    if (countryName.startsWith(p2pPrefix)) {
        return countryName.mid(p2pPrefix.size()) + " [P2P]";
    }
    return countryName;
}

void ApiCountryModel::rebuildGroupedRegions()
{
    QVector<RegionRowData> rows;
    const QString normalizedSearchText = normalizeSearchComparableText(m_searchText);
    const QStringList orderedRegions = {
        regionEurope,
        regionAmerica,
        regionAsia,
        regionOceaniaAfrica,
        regionOther,
    };

    QHash<QString, QVector<RegionRowData>> groupedCountries;
    for (int sourceIndex = 0; sourceIndex < m_countries.size(); ++sourceIndex) {
        const CountryInfo &sourceCountry = m_countries.at(sourceIndex);
        if (!isCountryMatchingSearch(sourceCountry.countryName, sourceCountry.countryCode, normalizedSearchText)) {
            continue;
        }

        const QString regionName = resolveRegionByIsoCode(extractCountryIsoCode(sourceCountry.countryCode));
        RegionRowData countryRow;
        countryRow.isRegionHeader = false;
        countryRow.regionName = regionName;
        countryRow.sourceIndex = sourceIndex;
        countryRow.countryName = getDisplayCountryName(sourceCountry.countryName);
        countryRow.sourceCountryName = sourceCountry.countryName;
        countryRow.countryCode = sourceCountry.countryCode;
        countryRow.countryImageCode = extractCountryIsoCode(sourceCountry.countryCode);
        groupedCountries[regionName].push_back(std::move(countryRow));
    }

    for (const QString &regionName : orderedRegions) {
        QVector<RegionRowData> countries = groupedCountries.value(regionName);
        if (countries.isEmpty()) {
            continue;
        }

        const bool expanded = isRegionExpanded(regionName);

        RegionRowData headerRow;
        headerRow.isRegionHeader = true;
        headerRow.regionName = regionName;
        headerRow.isExpanded = expanded;
        rows.push_back(std::move(headerRow));

        if (expanded) {
            for (RegionRowData &countryRow : countries) {
                countryRow.isExpanded = expanded;
                rows.push_back(std::move(countryRow));
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
