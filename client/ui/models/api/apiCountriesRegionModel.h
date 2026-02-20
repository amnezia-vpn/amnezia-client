#ifndef APICOUNTRIESREGIONMODEL_H
#define APICOUNTRIESREGIONMODEL_H

#include <QAbstractListModel>
#include <QHash>
#include <QVector>
#include <QString>

class ApiCountryModel;

class ApiCountriesRegionModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged)

public:
    enum Roles {
        RegionNameRole = Qt::UserRole + 1,
        CountriesRole
    };

    explicit ApiCountriesRegionModel(ApiCountryModel *sourceModel, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    QString searchText() const;
    void setSearchText(const QString &text);
    Q_INVOKABLE bool isRegionExpanded(const QString &regionName) const;
    Q_INVOKABLE void toggleRegionExpanded(const QString &regionName);

signals:
    void searchTextChanged();

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
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

    struct SourceCountry
    {
        QString countryName;
        QString countryCode;
        QString countryImageCode;
    };

    struct RegionData
    {
        QString regionName;
        QVariantList countries;
    };

    QString normalizeCountryCode(const QString &countryCode) const;
    QString extractCountryIsoCode(const QString &countryCode) const;
    QString normalizeCountryName(const QString &countryName) const;
    QString normalizeSearchComparableText(const QString &textValue) const;
    bool isCountryMatchingSearch(const QString &countryName, const QString &regionCountryCode, const QString &sourceCountryCode,
                                 const QString &ruCountryName) const;
    QString getDisplayCountryName(const QString &countryName) const;

    bool getSourceCountry(int sourceIndex, SourceCountry &country) const;
    int findCountryIndexByRef(const CountryRef &countryRef, const QHash<int, bool> &usedIndices) const;
    void rebuildModel();
    void loadRegionExpansionState();
    void saveRegionExpansionState() const;
    bool isSearchActive() const;

    QVector<RegionDefinition> m_regionDefinitions;
    QVector<RegionData> m_regions;
    ApiCountryModel *m_sourceModel = nullptr;
    QString m_searchText;
    QHash<QString, bool> m_regionsExpanded;
};

#endif // APICOUNTRIESREGIONMODEL_H
