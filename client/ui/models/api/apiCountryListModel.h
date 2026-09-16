#ifndef APICOUNTRYLISTMODEL_H
#define APICOUNTRYLISTMODEL_H

#include <QAbstractListModel>
#include <QHash>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QVector>

#include "core/utils/api/countryCatalog.h"

class ApiCountryModel;
class SecureAppSettingsRepository;

class ApiCountryListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        RowTypeRole = Qt::UserRole + 1, //!< "section" or "country"
        SectionKeyRole,
        SectionTitleCountRole,          //!< locations in the section, shown while it is collapsed
        IsSectionCollapsedRole,
        SourceIndexRole,
        CountryNameRole,       //!< what the row shows
        SourceCountryNameRole, //!< what the gateway called it, needed when switching location
        CountryCodeRole,
        CountryImageCodeRole,
        IsCurrentRole,
        IsIssuedRole,
        IsWorkerExpiredRole
    };

    enum SortMode {
        ByRegion = 0,
        Alphabetical = 1
    };
    Q_ENUM(SortMode)

    enum TabFilter {
        AllLocations = 0,
        AllowlistLocations = 1
    };
    Q_ENUM(TabFilter)

    explicit ApiCountryListModel(ApiCountryModel *source, SecureAppSettingsRepository *settings,
                                 const QString &listId, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged)
    Q_PROPERTY(int sortMode READ sortMode WRITE setSortMode NOTIFY sortModeChanged)
    Q_PROPERTY(int tabFilter READ tabFilter WRITE setTabFilter NOTIFY tabFilterChanged)
    Q_PROPERTY(bool hasAllowlistLocations READ hasAllowlistLocations NOTIFY layoutRebuilt)
    Q_PROPERTY(bool isSearchActive READ isSearchActive NOTIFY searchTextChanged)
    Q_PROPERTY(bool hasResults READ hasResults NOTIFY layoutRebuilt)
    Q_PROPERTY(bool isGrouped READ isGrouped NOTIFY sortModeChanged)
    Q_PROPERTY(int collapsedRevision READ collapsedRevision NOTIFY collapsedRevisionChanged)

    QString searchText() const;
    void setSearchText(const QString &text);

    int sortMode() const;
    void setSortMode(int mode);

    int tabFilter() const;
    void setTabFilter(int filter);
    bool hasAllowlistLocations() const;

    bool isSearchActive() const;
    bool hasResults() const;
    bool isGrouped() const;
    int collapsedRevision() const;

public slots:
    Q_INVOKABLE QString sectionRegionId(const QString &sectionKey) const;
    Q_INVOKABLE QString sectionSubregionId(const QString &sectionKey) const;
    Q_INVOKABLE int sectionCount(const QString &sectionKey) const;
    Q_INVOKABLE QString sectionKeyAtRow(int row) const;
    Q_INVOKABLE bool isSectionCollapsed(const QString &sectionKey) const;
    Q_INVOKABLE void toggleSection(const QString &sectionKey);
    Q_INVOKABLE void expandCurrentSection();
    Q_INVOKABLE void clearSearch();

signals:
    void searchTextChanged();
    void sortModeChanged();
    void tabFilterChanged();
    void layoutRebuilt();
    void collapsedRevisionChanged();

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    struct SearchIndex
    {
        QStringList exact;  //!< aliases and the ISO code, tight form
        QStringList spaced; //!< names and city, spaces kept for word-prefix matching
        QStringList tight;  //!< every field without spaces, for substring matching
    };

    struct Location
    {
        int sourceIndex = -1;
        QString regionId;
        QString subregionId;
        QString sectionKey;
        QString displayName;
        QString sourceName;
        QString countryCode;
        QString imageCode;
        bool countsTowardSplit = true;
        bool isAllowlist = false;
        SearchIndex search;
    };

    struct Row
    {
        bool isSectionHeader = false;
        int locationIndex = -1; //!< -1 on a header row
        QString sectionKey;
    };

    void reloadLocations();
    void rebuild();
    void notifyCollapsedChanged();
    void setSectionCollapsed(const QString &sectionKey, bool collapsed);

    QString buildSectionKey(const QString &regionId, const QString &subregionId) const;
    int matchLevel(const Location &location, const QString &spacedQuery, const QString &tightQuery) const;

    QPointer<ApiCountryModel> m_source;
    QPointer<SecureAppSettingsRepository> m_settings;
    QString m_listId;
    countryCatalog::Catalog m_catalog;

    QVector<Location> m_locations;
    QVector<Row> m_rows;
    QHash<QString, int> m_sectionCounts;
    QHash<QString, QVector<int>> m_sectionOrder; //!< location indexes per section, in display order
    QHash<QString, bool> m_collapsedSections; //!< session only, deliberately not persisted

    QString m_searchText;
    int m_sortMode = ByRegion;
    int m_tabFilter = AllLocations;
    int m_collapsedRevision = 0;
};

#endif // APICOUNTRYLISTMODEL_H
