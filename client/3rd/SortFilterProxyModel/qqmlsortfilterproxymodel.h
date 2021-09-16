#ifndef QQMLSORTFILTERPROXYMODEL_H
#define QQMLSORTFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>
#include <QQmlParserStatus>
#include "filters/filtercontainer.h"
#include "sorters/sortercontainer.h"
#include "proxyroles/proxyrolecontainer.h"

namespace qqsfpm {

class QQmlSortFilterProxyModel : public QSortFilterProxyModel,
                                 public QQmlParserStatus,
                                 public FilterContainer,
                                 public SorterContainer,
                                 public ProxyRoleContainer
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_INTERFACES(qqsfpm::FilterContainer)
    Q_INTERFACES(qqsfpm::SorterContainer)
    Q_INTERFACES(qqsfpm::ProxyRoleContainer)

    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(bool delayed READ delayed WRITE setDelayed NOTIFY delayedChanged)

    Q_PROPERTY(QString filterRoleName READ filterRoleName WRITE setFilterRoleName NOTIFY filterRoleNameChanged)
    Q_PROPERTY(QString filterPattern READ filterPattern WRITE setFilterPattern NOTIFY filterPatternChanged)
    Q_PROPERTY(PatternSyntax filterPatternSyntax READ filterPatternSyntax WRITE setFilterPatternSyntax NOTIFY filterPatternSyntaxChanged)
    Q_PROPERTY(QVariant filterValue READ filterValue WRITE setFilterValue NOTIFY filterValueChanged)

    Q_PROPERTY(QString sortRoleName READ sortRoleName WRITE setSortRoleName NOTIFY sortRoleNameChanged)
    Q_PROPERTY(bool ascendingSortOrder READ ascendingSortOrder WRITE setAscendingSortOrder NOTIFY ascendingSortOrderChanged)

    Q_PROPERTY(QQmlListProperty<qqsfpm::Filter> filters READ filtersListProperty)
    Q_PROPERTY(QQmlListProperty<qqsfpm::Sorter> sorters READ sortersListProperty)
    Q_PROPERTY(QQmlListProperty<qqsfpm::ProxyRole> proxyRoles READ proxyRolesListProperty)

public:
    enum PatternSyntax {
        RegExp = QRegExp::RegExp,
        Wildcard = QRegExp::Wildcard,
        FixedString = QRegExp::FixedString,
        RegExp2 = QRegExp::RegExp2,
        WildcardUnix = QRegExp::WildcardUnix,
        W3CXmlSchema11 = QRegExp::W3CXmlSchema11 };
    Q_ENUMS(PatternSyntax)

    QQmlSortFilterProxyModel(QObject* parent = 0);

    int count() const;

    bool delayed() const;
    void setDelayed(bool delayed);

    const QString& filterRoleName() const;
    void setFilterRoleName(const QString& filterRoleName);

    QString filterPattern() const;
    void setFilterPattern(const QString& filterPattern);

    PatternSyntax filterPatternSyntax() const;
    void setFilterPatternSyntax(PatternSyntax patternSyntax);

    const QVariant& filterValue() const;
    void setFilterValue(const QVariant& filterValue);

    const QString& sortRoleName() const;
    void setSortRoleName(const QString& sortRoleName);

    bool ascendingSortOrder() const;
    void setAscendingSortOrder(bool ascendingSortOrder);

    void classBegin() override;
    void componentComplete() override;

    QVariant sourceData(const QModelIndex& sourceIndex, const QString& roleName) const;
    QVariant sourceData(const QModelIndex& sourceIndex, int role) const;

    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE int roleForName(const QString& roleName) const;

    Q_INVOKABLE QVariantMap get(int row) const;
    Q_INVOKABLE QVariant get(int row, const QString& roleName) const;

    Q_INVOKABLE QModelIndex mapToSource(const QModelIndex& proxyIndex) const override;
    Q_INVOKABLE int mapToSource(int proxyRow) const;
    Q_INVOKABLE QModelIndex mapFromSource(const QModelIndex& sourceIndex) const override;
    Q_INVOKABLE int mapFromSource(int sourceRow) const;

    void setSourceModel(QAbstractItemModel *sourceModel) override;

Q_SIGNALS:
    void countChanged();
    void delayedChanged();

    void filterRoleNameChanged();
    void filterPatternSyntaxChanged();
    void filterPatternChanged();
    void filterValueChanged();

    void sortRoleNameChanged();
    void ascendingSortOrderChanged();

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;
    bool lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const override;

protected Q_SLOTS:
    void resetInternalData();

private Q_SLOTS:
    void queueInvalidateFilter();
    void invalidateFilter();
    void queueInvalidate();
    void invalidate();
    void updateRoleNames();
    void updateFilterRole();
    void updateSortRole();
    void updateRoles();
    void initRoles();
    void onDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles);
    void queueInvalidateProxyRoles();
    void invalidateProxyRoles();

private:
    QVariantMap modelDataMap(const QModelIndex& modelIndex) const;

    void onFilterAppended(Filter* filter) override;
    void onFilterRemoved(Filter* filter) override;
    void onFiltersCleared() override;

    void onSorterAppended(Sorter* sorter) override;
    void onSorterRemoved(Sorter* sorter) override;
    void onSortersCleared() override;

    void onProxyRoleAppended(ProxyRole *proxyRole) override;
    void onProxyRoleRemoved(ProxyRole *proxyRole) override;
    void onProxyRolesCleared() override;

    bool m_delayed;
    QString m_filterRoleName;
    QVariant m_filterValue;
    QString m_sortRoleName;
    bool m_ascendingSortOrder = true;
    bool m_completed = false;
    QHash<int, QByteArray> m_roleNames;
    QHash<int, QPair<ProxyRole*, QString>> m_proxyRoleMap;
    QVector<int> m_proxyRoleNumbers;

    bool m_invalidateFilterQueued = false;
    bool m_invalidateQueued = false;
    bool m_invalidateProxyRolesQueued = false;
};

}

#endif // QQMLSORTFILTERPROXYMODEL_H
