#ifndef APICOUNTRYMODEL_H
#define APICOUNTRYMODEL_H

#include <QAbstractListModel>
#include <QHash>
#include <QJsonArray>
#include <memory>

class ApiCountryModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        CountryNameRole = Qt::UserRole + 1,
        CountryCodeRole,
        CountryImageCodeRole,
        IsIssuedRole,
        IsWorkerExpiredRole
    };

    explicit ApiCountryModel(QObject *parent = nullptr);
    ~ApiCountryModel() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    Q_PROPERTY(int currentIndex READ getCurrentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged)
    Q_PROPERTY(QAbstractListModel *regionRowsModel READ regionRowsModel CONSTANT)
    Q_PROPERTY(bool hasVisibleRegions READ hasVisibleRegions NOTIFY regionRowsChanged)

public slots:
    void updateModel(const QJsonArray &countries, const QString &currentCountryCode);
    void updateIssuedConfigsInfo(const QJsonArray &issuedConfigs);

    int getCurrentIndex();
    void setCurrentIndex(const int i);
    QString searchText() const;
    void setSearchText(const QString &text);
    QAbstractListModel *regionRowsModel() const;
    bool hasVisibleRegions() const;
    Q_INVOKABLE bool isRegionExpanded(const QString &regionName) const;
    Q_INVOKABLE void toggleRegionExpanded(const QString &regionName);

signals:
    void currentIndexChanged(const int index);
    void searchTextChanged();
    void regionRowsChanged();

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    struct IssuedConfigInfo
    {
        QString installationUuid;
        QString workerLastUpdated;
        QString lastDownloaded;
        QString sourceType;
        QString osVersion;
    };

    struct CountryInfo
    {
        QString countryName;
        QString countryCode;
    };

    struct CountryRef
    {
        QString code;
        QString name;
        QString ruName;
    };

    struct RegionDefinition
    {
        QString regionName;
        QVector<CountryRef> countries;
    };

    QVector<CountryInfo> m_countries;
    QHash<QString, IssuedConfigInfo> m_issuedConfigs;
    int m_currentIndex = -1;
    QString m_searchText;
    QVector<RegionDefinition> m_regionDefinitions;
    QHash<QString, bool> m_regionsExpanded;
    class RegionRowsModel;
    std::unique_ptr<RegionRowsModel> m_regionRowsModel;

    QString normalizeCountryCode(const QString &countryCode) const;
    QString extractCountryIsoCode(const QString &countryCode) const;
    QString normalizeCountryName(const QString &countryName) const;
    QString normalizeSearchComparableText(const QString &textValue) const;
    bool isCountryMatchingSearch(const QString &countryName, const QString &regionCountryCode, const QString &sourceCountryCode,
                                 const QString &ruCountryName, const QString &normalizedSearchText) const;
    QString getDisplayCountryName(const QString &countryName) const;
    int findCountryIndexByRef(const CountryRef &countryRef, const QHash<int, bool> &usedIndices) const;
    void rebuildGroupedRegions();
    void loadRegionExpansionState();
    void saveRegionExpansionState() const;
    bool isSearchActive() const;
};

#endif // APICOUNTRYMODEL_H
