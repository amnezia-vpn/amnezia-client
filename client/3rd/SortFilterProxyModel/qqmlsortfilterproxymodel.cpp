#include "qqmlsortfilterproxymodel.h"
#include <QtQml>
#include <algorithm>
#include "filters/filter.h"
#include "sorters/sorter.h"
#include "proxyroles/proxyrole.h"

namespace qqsfpm {

/*!
    \qmltype SortFilterProxyModel
    \inqmlmodule SortFilterProxyModel
    \ingroup SortFilterProxyModel
    \ingroup FilterContainer
    \ingroup SorterContainer
    \brief Filters and sorts data coming from a source \l {http://doc.qt.io/qt-5/qabstractitemmodel.html} {QAbstractItemModel}.

    The SortFilterProxyModel type provides support for filtering and sorting data coming from a source model.
    \sa FilterContainer, SorterContainer
*/
QQmlSortFilterProxyModel::QQmlSortFilterProxyModel(QObject *parent) :
    QSortFilterProxyModel(parent),
#ifdef SFPM_DELAYED
    m_delayed(true)
#else
    m_delayed(false)
#endif
{
    connect(this, &QAbstractProxyModel::sourceModelChanged, this, &QQmlSortFilterProxyModel::updateRoles);
    connect(this, &QAbstractItemModel::modelReset, this, &QQmlSortFilterProxyModel::updateRoles);
    connect(this, &QAbstractItemModel::rowsInserted, this, &QQmlSortFilterProxyModel::countChanged);
    connect(this, &QAbstractItemModel::rowsRemoved, this, &QQmlSortFilterProxyModel::countChanged);
    connect(this, &QAbstractItemModel::modelReset, this, &QQmlSortFilterProxyModel::countChanged);
    connect(this, &QAbstractItemModel::layoutChanged, this, &QQmlSortFilterProxyModel::countChanged);
    connect(this, &QAbstractItemModel::dataChanged, this, &QQmlSortFilterProxyModel::onDataChanged);
    setDynamicSortFilter(true);
}

/*!
    \qmlproperty QAbstractItemModel* SortFilterProxyModel::sourceModel

    The source model of this proxy model
*/

/*!
    \qmlproperty int SortFilterProxyModel::count

    The number of rows in the proxy model (not filtered out the source model)
*/
int QQmlSortFilterProxyModel::count() const
{
    return rowCount();
}

/*!
    \qmlproperty bool SortFilterProxyModel::delayed

    Delay the execution of filters, sorters and proxyRoles until the next event loop.
    This can be used as an optimization when multiple filters, sorters or proxyRoles are changed in a single event loop.
    They will be executed once in a single batch at the next event loop instead of being executed in multiple sequential batches.

    By default, the SortFilterProxyModel is not delayed, unless the \c SFPM_DELAYED environment variable is defined at compile time.
*/
bool QQmlSortFilterProxyModel::delayed() const
{
    return m_delayed;
}

void QQmlSortFilterProxyModel::setDelayed(bool delayed)
{
    if (m_delayed == delayed)
        return;

    m_delayed = delayed;
    Q_EMIT delayedChanged();
}

const QString& QQmlSortFilterProxyModel::filterRoleName() const
{
    return m_filterRoleName;
}

void QQmlSortFilterProxyModel::setFilterRoleName(const QString& filterRoleName)
{
    if (m_filterRoleName == filterRoleName)
        return;

    m_filterRoleName = filterRoleName;
    updateFilterRole();
    Q_EMIT filterRoleNameChanged();
}

QString QQmlSortFilterProxyModel::filterPattern() const
{
    return filterRegExp().pattern();
}

void QQmlSortFilterProxyModel::setFilterPattern(const QString& filterPattern)
{
    QRegExp regExp = filterRegExp();
    if (regExp.pattern() == filterPattern)
        return;

    regExp.setPattern(filterPattern);
    QSortFilterProxyModel::setFilterRegExp(regExp);
    Q_EMIT filterPatternChanged();
}

QQmlSortFilterProxyModel::PatternSyntax QQmlSortFilterProxyModel::filterPatternSyntax() const
{
    return static_cast<PatternSyntax>(filterRegExp().patternSyntax());
}

void QQmlSortFilterProxyModel::setFilterPatternSyntax(QQmlSortFilterProxyModel::PatternSyntax patternSyntax)
{
    QRegExp regExp = filterRegExp();
    QRegExp::PatternSyntax patternSyntaxTmp = static_cast<QRegExp::PatternSyntax>(patternSyntax);
    if (regExp.patternSyntax() == patternSyntaxTmp)
        return;

    regExp.setPatternSyntax(patternSyntaxTmp);
    QSortFilterProxyModel::setFilterRegExp(regExp);
    Q_EMIT filterPatternSyntaxChanged();
}

const QVariant& QQmlSortFilterProxyModel::filterValue() const
{
    return m_filterValue;
}

void QQmlSortFilterProxyModel::setFilterValue(const QVariant& filterValue)
{
    if (m_filterValue == filterValue)
        return;

    m_filterValue = filterValue;
    queueInvalidateFilter();
    Q_EMIT filterValueChanged();
}

/*!
    \qmlproperty string SortFilterProxyModel::sortRoleName

    The role name of the source model's data used for the sorting.

    \sa {http://doc.qt.io/qt-5/qsortfilterproxymodel.html#sortRole-prop} {sortRole}, roleForName
*/
const QString& QQmlSortFilterProxyModel::sortRoleName() const
{
    return m_sortRoleName;
}

void QQmlSortFilterProxyModel::setSortRoleName(const QString& sortRoleName)
{
    if (m_sortRoleName == sortRoleName)
        return;

    m_sortRoleName = sortRoleName;
    updateSortRole();
    Q_EMIT sortRoleNameChanged();
}

bool QQmlSortFilterProxyModel::ascendingSortOrder() const
{
    return m_ascendingSortOrder;
}

void QQmlSortFilterProxyModel::setAscendingSortOrder(bool ascendingSortOrder)
{
    if (m_ascendingSortOrder == ascendingSortOrder)
        return;

    m_ascendingSortOrder = ascendingSortOrder;
    Q_EMIT ascendingSortOrderChanged();
    queueInvalidate();
}

/*!
    \qmlproperty list<Filter> SortFilterProxyModel::filters

    This property holds the list of filters for this proxy model. To be included in the model, a row of the source model has to be accepted by all the top level filters of this list.

    \sa Filter, FilterContainer
*/

/*!
    \qmlproperty list<Sorter> SortFilterProxyModel::sorters

    This property holds the list of sorters for this proxy model. The rows of the source model are sorted by the sorters of this list, in their order of insertion.

    \sa Sorter, SorterContainer
*/

/*!
    \qmlproperty list<ProxyRole> SortFilterProxyModel::proxyRoles

    This property holds the list of proxy roles for this proxy model. Each proxy role adds a new custom role to the model.

    \sa ProxyRole
*/

void QQmlSortFilterProxyModel::classBegin()
{

}

void QQmlSortFilterProxyModel::componentComplete()
{
    m_completed = true;

    for (const auto& filter : m_filters)
        filter->proxyModelCompleted(*this);
    for (const auto& sorter : m_sorters)
        sorter->proxyModelCompleted(*this);
    for (const auto& proxyRole : m_proxyRoles)
        proxyRole->proxyModelCompleted(*this);

    invalidate();
    sort(0);
}

QVariant QQmlSortFilterProxyModel::sourceData(const QModelIndex& sourceIndex, const QString& roleName) const
{
    int role = roleNames().key(roleName.toUtf8());
    return sourceData(sourceIndex, role);
}

QVariant QQmlSortFilterProxyModel::sourceData(const QModelIndex &sourceIndex, int role) const
{
    QPair<ProxyRole*, QString> proxyRolePair = m_proxyRoleMap[role];
    if (ProxyRole* proxyRole = proxyRolePair.first)
        return proxyRole->roleData(sourceIndex, *this, proxyRolePair.second);
    else
        return sourceModel()->data(sourceIndex, role);
}

QVariant QQmlSortFilterProxyModel::data(const QModelIndex &index, int role) const
{
    return sourceData(mapToSource(index), role);
}

QHash<int, QByteArray> QQmlSortFilterProxyModel::roleNames() const
{
    return m_roleNames.isEmpty() && sourceModel() ? sourceModel()->roleNames() : m_roleNames;
}

/*!
    \qmlmethod int SortFilterProxyModel::roleForName(string roleName)

    Returns the role number for the given \a roleName.
    If no role is found for this \a roleName, \c -1 is returned.
*/

int QQmlSortFilterProxyModel::roleForName(const QString& roleName) const
{
    return m_roleNames.key(roleName.toUtf8(), -1);
}

/*!
    \qmlmethod object SortFilterProxyModel::get(int row)

    Return the item at \a row in the proxy model as a map of all its roles. This allows the item data to be read (not modified) from JavaScript.
*/
QVariantMap QQmlSortFilterProxyModel::get(int row) const
{
    QVariantMap map;
    QModelIndex modelIndex = index(row, 0);
    QHash<int, QByteArray> roles = roleNames();
    for (QHash<int, QByteArray>::const_iterator it = roles.begin(); it != roles.end(); ++it)
        map.insert(it.value(), data(modelIndex, it.key()));
    return map;
}

/*!
    \qmlmethod variant SortFilterProxyModel::get(int row, string roleName)

    Return the data for the given \a roleName of the item at \a row in the proxy model. This allows the role data to be read (not modified) from JavaScript.
    This equivalent to calling \c {data(index(row, 0), roleForName(roleName))}.
*/
QVariant QQmlSortFilterProxyModel::get(int row, const QString& roleName) const
{
    return data(index(row, 0), roleForName(roleName));
}

/*!
    \qmlmethod index SortFilterProxyModel::mapToSource(index proxyIndex)

    Returns the source model index corresponding to the given \a proxyIndex from the SortFilterProxyModel.
*/
QModelIndex QQmlSortFilterProxyModel::mapToSource(const QModelIndex& proxyIndex) const
{
    return QSortFilterProxyModel::mapToSource(proxyIndex);
}

/*!
    \qmlmethod int SortFilterProxyModel::mapToSource(int proxyRow)

    Returns the source model row corresponding to the given \a proxyRow from the SortFilterProxyModel.
    Returns -1 if there is no corresponding row.
*/
int QQmlSortFilterProxyModel::mapToSource(int proxyRow) const
{
    QModelIndex proxyIndex = index(proxyRow, 0);
    QModelIndex sourceIndex = mapToSource(proxyIndex);
    return sourceIndex.isValid() ? sourceIndex.row() : -1;
}

/*!
    \qmlmethod QModelIndex SortFilterProxyModel::mapFromSource(QModelIndex sourceIndex)

    Returns the model index in the SortFilterProxyModel given the \a sourceIndex from the source model.
*/
QModelIndex QQmlSortFilterProxyModel::mapFromSource(const QModelIndex& sourceIndex) const
{
    return QSortFilterProxyModel::mapFromSource(sourceIndex);
}

/*!
    \qmlmethod int SortFilterProxyModel::mapFromSource(int sourceRow)

    Returns the row in the SortFilterProxyModel given the \a sourceRow from the source model.
    Returns -1 if there is no corresponding row.
*/
int QQmlSortFilterProxyModel::mapFromSource(int sourceRow) const
{
    QModelIndex proxyIndex;
    if (QAbstractItemModel* source = sourceModel()) {
        QModelIndex sourceIndex = source->index(sourceRow, 0);
        proxyIndex = mapFromSource(sourceIndex);
    }
    return proxyIndex.isValid() ? proxyIndex.row() : -1;
}

bool QQmlSortFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
{
    if (!m_completed)
        return true;
    QModelIndex sourceIndex = sourceModel()->index(source_row, 0, source_parent);
    bool valueAccepted = !m_filterValue.isValid() || ( m_filterValue == sourceModel()->data(sourceIndex, filterRole()) );
    bool baseAcceptsRow = valueAccepted && QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
    baseAcceptsRow = baseAcceptsRow && std::all_of(m_filters.begin(), m_filters.end(),
        [=, &source_parent] (Filter* filter) {
            return filter->filterAcceptsRow(sourceIndex, *this);
        }
    );
    return baseAcceptsRow;
}

bool QQmlSortFilterProxyModel::lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const
{
    if (m_completed) {
        if (!m_sortRoleName.isEmpty()) {
            if (QSortFilterProxyModel::lessThan(source_left, source_right))
                return m_ascendingSortOrder;
            if (QSortFilterProxyModel::lessThan(source_right, source_left))
                return !m_ascendingSortOrder;
        }
        auto sortedSorters = m_sorters;
        std::stable_sort(sortedSorters.begin(),
                         sortedSorters.end(),
                         [] (Sorter* a, Sorter* b) {
                             return a->priority() > b->priority();
                         });
        for(auto sorter : sortedSorters) {
            if (sorter->enabled()) {
                int comparison = sorter->compareRows(source_left, source_right, *this);
                if (comparison != 0)
                    return comparison < 0;
            }
        }
    }
    return source_left.row() < source_right.row();
}

void QQmlSortFilterProxyModel::resetInternalData()
{
    QSortFilterProxyModel::resetInternalData();
    updateRoleNames();
}

void QQmlSortFilterProxyModel::setSourceModel(QAbstractItemModel *sourceModel)
{
    if (sourceModel && sourceModel->roleNames().isEmpty()) { // workaround for when a model has no roles and roles are added when the model is populated (ListModel)
        // QTBUG-57971
        connect(sourceModel, &QAbstractItemModel::rowsInserted, this, &QQmlSortFilterProxyModel::initRoles);
    }
    QSortFilterProxyModel::setSourceModel(sourceModel);
}

void QQmlSortFilterProxyModel::queueInvalidateFilter()
{
    if (m_delayed) {
        if (!m_invalidateFilterQueued && !m_invalidateQueued) {
            m_invalidateFilterQueued = true;
            QMetaObject::invokeMethod(this, "invalidateFilter", Qt::QueuedConnection);
        }
    } else {
        invalidateFilter();
    }
}

void QQmlSortFilterProxyModel::invalidateFilter()
{
    m_invalidateFilterQueued = false;
    if (m_completed && !m_invalidateQueued)
        QSortFilterProxyModel::invalidateFilter();
}

void QQmlSortFilterProxyModel::queueInvalidate()
{
    if (m_delayed) {
        if (!m_invalidateQueued) {
            m_invalidateQueued = true;
            QMetaObject::invokeMethod(this, "invalidate", Qt::QueuedConnection);
        }
    } else {
        invalidate();
    }
}

void QQmlSortFilterProxyModel::invalidate()
{
    m_invalidateQueued = false;
    if (m_completed)
        QSortFilterProxyModel::invalidate();
}

void QQmlSortFilterProxyModel::updateRoleNames()
{
    if (!sourceModel())
        return;
    m_roleNames = sourceModel()->roleNames();
    m_proxyRoleMap.clear();
    m_proxyRoleNumbers.clear();

    auto roles = m_roleNames.keys();
    auto maxIt = std::max_element(roles.cbegin(), roles.cend());
    int maxRole = maxIt != roles.cend() ? *maxIt : -1;
    for (auto proxyRole : m_proxyRoles) {
        for (auto roleName : proxyRole->names()) {
            ++maxRole;
            m_roleNames[maxRole] = roleName.toUtf8();
            m_proxyRoleMap[maxRole] = {proxyRole, roleName};
            m_proxyRoleNumbers.append(maxRole);
        }
    }
}

void QQmlSortFilterProxyModel::updateFilterRole()
{
    QList<int> filterRoles = roleNames().keys(m_filterRoleName.toUtf8());
    if (!filterRoles.empty())
    {
        setFilterRole(filterRoles.first());
    }
}

void QQmlSortFilterProxyModel::updateSortRole()
{
    QList<int> sortRoles = roleNames().keys(m_sortRoleName.toUtf8());
    if (!sortRoles.empty())
    {
        setSortRole(sortRoles.first());
        queueInvalidate();
    }
}

void QQmlSortFilterProxyModel::updateRoles()
{
    updateFilterRole();
    updateSortRole();
}

void QQmlSortFilterProxyModel::initRoles()
{
    disconnect(sourceModel(), &QAbstractItemModel::rowsInserted, this, &QQmlSortFilterProxyModel::initRoles);
    resetInternalData();
    updateRoles();
}

void QQmlSortFilterProxyModel::onDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
{
    Q_UNUSED(roles);
    if (!roles.isEmpty() && !m_proxyRoleNumbers.empty() && roles != m_proxyRoleNumbers)
        Q_EMIT dataChanged(topLeft, bottomRight, m_proxyRoleNumbers);
}

void QQmlSortFilterProxyModel::queueInvalidateProxyRoles()
{
    queueInvalidate();
    if (m_delayed) {
        if (!m_invalidateProxyRolesQueued) {
            m_invalidateProxyRolesQueued = true;
            QMetaObject::invokeMethod(this, "invalidateProxyRoles", Qt::QueuedConnection);
        }
    } else {
        invalidateProxyRoles();
    }
}

void QQmlSortFilterProxyModel::invalidateProxyRoles()
{
    m_invalidateProxyRolesQueued = false;
    if (m_completed)
        Q_EMIT dataChanged(index(0,0), index(rowCount() - 1, columnCount() - 1), m_proxyRoleNumbers);
}

QVariantMap QQmlSortFilterProxyModel::modelDataMap(const QModelIndex& modelIndex) const
{
    QVariantMap map;
    QHash<int, QByteArray> roles = roleNames();
    for (QHash<int, QByteArray>::const_iterator it = roles.begin(); it != roles.end(); ++it)
        map.insert(it.value(), sourceModel()->data(modelIndex, it.key()));
    return map;
}

void QQmlSortFilterProxyModel::onFilterAppended(Filter* filter)
{
    connect(filter, &Filter::invalidated, this, &QQmlSortFilterProxyModel::queueInvalidateFilter);
    queueInvalidateFilter();
}

void QQmlSortFilterProxyModel::onFilterRemoved(Filter* filter)
{
    Q_UNUSED(filter)
    queueInvalidateFilter();
}

void QQmlSortFilterProxyModel::onFiltersCleared()
{
    queueInvalidateFilter();
}

void QQmlSortFilterProxyModel::onSorterAppended(Sorter* sorter)
{
    connect(sorter, &Sorter::invalidated, this, &QQmlSortFilterProxyModel::queueInvalidate);
    queueInvalidate();
}

void QQmlSortFilterProxyModel::onSorterRemoved(Sorter* sorter)
{
    Q_UNUSED(sorter)
    queueInvalidate();
}

void QQmlSortFilterProxyModel::onSortersCleared()
{
    queueInvalidate();
}

void QQmlSortFilterProxyModel::onProxyRoleAppended(ProxyRole *proxyRole)
{
    beginResetModel();
    connect(proxyRole, &ProxyRole::invalidated, this, &QQmlSortFilterProxyModel::queueInvalidateProxyRoles);
    connect(proxyRole, &ProxyRole::namesAboutToBeChanged, this, &QQmlSortFilterProxyModel::beginResetModel);
    connect(proxyRole, &ProxyRole::namesChanged, this, &QQmlSortFilterProxyModel::endResetModel);
    endResetModel();
}

void QQmlSortFilterProxyModel::onProxyRoleRemoved(ProxyRole *proxyRole)
{
    Q_UNUSED(proxyRole)
    beginResetModel();
    endResetModel();
}

void QQmlSortFilterProxyModel::onProxyRolesCleared()
{
    beginResetModel();
    endResetModel();
}

void registerQQmlSortFilterProxyModelTypes() {
    qmlRegisterType<QQmlSortFilterProxyModel>("SortFilterProxyModel", 0, 2, "SortFilterProxyModel");
}

Q_COREAPP_STARTUP_FUNCTION(registerQQmlSortFilterProxyModelTypes)

}
