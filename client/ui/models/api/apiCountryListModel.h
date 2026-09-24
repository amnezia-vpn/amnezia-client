#ifndef APICOUNTRYLISTMODEL_H
#define APICOUNTRYLISTMODEL_H

#include <QAbstractListModel>
#include <QHash>
#include <QPointer>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVector>

#include "core/utils/api/countryCatalog.h"

class ApiCountryModel;

class ApiCountryListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        RowTypeRole = Qt::UserRole + 1,
        SectionKeyRole,
        SourceIndexRole,
        CountryNameRole,
        SourceCountryNameRole,
        CountryCodeRole,
        CountryImageCodeRole,
        IsCurrentRole,
        IsIssuedRole,
        IsWorkerExpiredRole,
        IsFavoriteRole
    };

    enum SortMode {
        ByRegion = 0,
        Alphabetical = 1
    };
    Q_ENUM(SortMode)

    enum class UseCaseSet {
        Connection,
        ConfigFiles
    };

    explicit ApiCountryListModel(ApiCountryModel *source, const QString &listId,
                                 QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged)
    Q_PROPERTY(int sortMode READ sortMode WRITE setSortMode NOTIFY sortModeChanged)
    Q_PROPERTY(bool isSearchActive READ isSearchActive NOTIFY searchTextChanged)
    Q_PROPERTY(bool hasResults READ hasResults NOTIFY layoutRebuilt)
    Q_PROPERTY(bool isGrouped READ isGrouped NOTIFY groupingChanged)
    Q_PROPERTY(int collapsedRevision READ collapsedRevision NOTIFY collapsedRevisionChanged)
    Q_PROPERTY(int layoutRevision READ layoutRevision NOTIFY layoutRebuilt)
    Q_PROPERTY(QString activeUseCaseId READ activeUseCaseId WRITE setActiveUseCaseId NOTIFY activeUseCaseIdChanged)
    Q_PROPERTY(QVariantList useCases READ useCases NOTIFY useCasesChanged)
    Q_PROPERTY(int favoritesLimit READ favoritesLimit CONSTANT)

    QString searchText() const;
    void setSearchText(const QString &text);

    int sortMode() const;
    void setSortMode(int mode);

    QString listId() const;

    bool isSearchActive() const;
    bool hasResults() const;
    bool isGrouped() const;
    int collapsedRevision() const;
    int layoutRevision() const;

    QString activeUseCaseId() const;
    void setActiveUseCaseId(const QString &id);
    QVariantList useCases() const;
    int favoritesLimit() const;
    int catalogVersion() const;

    QStringList favorites() const;
    void setFavorites(const QStringList &codes);
    QStringList collapsedSections() const;
    void setCollapsedSections(const QStringList &keys);

public slots:
    Q_INVOKABLE QString sectionRegionId(const QString &sectionKey) const;
    Q_INVOKABLE QString sectionSubregionId(const QString &sectionKey) const;
    Q_INVOKABLE QString sectionSubsubregionId(const QString &sectionKey) const;
    Q_INVOKABLE int sectionCount(const QString &sectionKey) const;
    Q_INVOKABLE QString sectionKeyAtRow(int row) const;
    Q_INVOKABLE bool isSectionCollapsed(const QString &sectionKey) const;
    Q_INVOKABLE void toggleSection(const QString &sectionKey);
    Q_INVOKABLE void expandCurrentSection();
    Q_INVOKABLE void clearSearch();

    Q_INVOKABLE bool toggleFavorite(const QString &countryCode);
    Q_INVOKABLE void applyDefaultState(bool followCurrentLocation);
    Q_INVOKABLE int rowForCountryCode(const QString &countryCode) const;
    Q_INVOKABLE bool isSectionHeaderRow(int row) const;
    Q_INVOKABLE int rowForSectionHeader(const QString &sectionKey) const;

signals:
    void searchTextChanged();
    void sortModeChanged();
    void groupingChanged();
    void layoutRebuilt();
    void collapsedRevisionChanged();
    void activeUseCaseIdChanged();
    void useCasesChanged();
    void favoritesChanged(const QStringList &codes);
    void favoritesLimitExceeded();
    void collapsedSectionsChanged(const QStringList &keys);
    void positionRequested(int row);
    void sourceAboutToRefresh();
    void sourceRefreshed();

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    struct SearchIndex
    {
        QStringList exact;
        QStringList spaced;
        QStringList tight;
    };

    struct Location
    {
        int sourceIndex = -1;
        QString regionId;
        QString subregionId;
        QString subsubregionId;
        QString sectionKey;
        QString displayName;
        QString sourceName;
        QString countryCode;
        QString imageCode;
        SearchIndex search;
    };

    struct Row
    {
        bool isSectionHeader = false;
        int locationIndex = -1;
        QString sectionKey;

        bool operator==(const Row &other) const
        {
            return isSectionHeader == other.isSectionHeader && locationIndex == other.locationIndex
                    && sectionKey == other.sectionKey;
        }
    };

    void reloadLocations();
    void rebuild();
    QVector<Row> buildRows() const;
    void applyRows(const QVector<Row> &next);
    void notifyCollapsedChanged();
    void setSectionCollapsed(const QString &sectionKey, bool collapsed);

    QString buildSectionKey(const QString &regionId, const QString &subregionId,
                            const QString &subsubregionId = QString()) const;
    QString parentSectionKey(const QString &sectionKey) const;
    bool isHiddenByParent(const QString &sectionKey) const;

    bool passesActiveUseCase(const Location &location) const;
    bool isIssued(const Location &location) const;
    bool isUseCaseVisible(const countryCatalog::UseCase &useCase) const;
    const countryCatalog::UseCase *findUseCase(const QString &id) const;
    bool rebuildUseCases();
    void emitCollapsedSections();
    int matchLevel(const Location &location, const QString &spacedQuery, const QString &tightQuery) const;

    QPointer<ApiCountryModel> m_source;
    QString m_listId;
    countryCatalog::Catalog m_catalog;

    QVector<Location> m_locations;
    QVector<Row> m_rows;
    QHash<QString, int> m_sectionCounts;
    QHash<QString, QVector<int>> m_sectionOrder;
    QStringList m_orderedSectionKeys;
    QHash<QString, bool> m_collapsedSections;

    QString m_searchText;
    int m_sortMode = ByRegion;
    int m_collapsedRevision = 0;
    int m_layoutRevision = 0;

    UseCaseSet m_useCaseSet = UseCaseSet::Connection;
    QString m_activeUseCaseId;
    QVariantList m_useCases;

    QSet<QString> m_favorites;
    QSet<QString> m_favoritesSnapshot;
};

#endif
