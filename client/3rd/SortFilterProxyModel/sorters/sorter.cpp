#include "sorter.h"
#include "qqmlsortfilterproxymodel.h"

namespace qqsfpm {

/*!
    \qmltype Sorter
    \qmlabstract
    \inqmlmodule SortFilterProxyModel
    \ingroup Sorters
    \brief Base type for the \l SortFilterProxyModel sorters.

    The Sorter type cannot be used directly in a QML file.
    It exists to provide a set of common properties and methods,
    available across all the other sorters types that inherit from it.
    Attempting to use the Sorter type directly will result in an error.
*/

Sorter::Sorter(QObject *parent) : QObject(parent)
{
}

Sorter::~Sorter() = default;

/*!
    \qmlproperty bool Sorter::enabled

    This property holds whether the sorter is enabled.
    A disabled sorter will not change the order of the rows.

    By default, sorters are enabled.
*/
bool Sorter::enabled() const
{
    return m_enabled;
}

void Sorter::setEnabled(bool enabled)
{
    if (m_enabled == enabled)
        return;

    m_enabled = enabled;
    Q_EMIT enabledChanged();
    Q_EMIT invalidated();
}

bool Sorter::ascendingOrder() const
{
    return sortOrder() == Qt::AscendingOrder;
}

void Sorter::setAscendingOrder(bool ascendingOrder)
{
    setSortOrder(ascendingOrder ? Qt::AscendingOrder : Qt::DescendingOrder);
}


/*!
    \qmlproperty Qt::SortOrder Sorter::sortOrder

    This property holds the sort order of this sorter.

    \value Qt.AscendingOrder The items are sorted ascending e.g. starts with 'AAA' ends with 'ZZZ' in Latin-1 locales
    \value Qt.DescendingOrder The items are sorted descending e.g. starts with 'ZZZ' ends with 'AAA' in Latin-1 locales

    By default, sorting is in ascending order.
*/
Qt::SortOrder Sorter::sortOrder() const
{
    return m_sortOrder;
}

void Sorter::setSortOrder(Qt::SortOrder sortOrder)
{
    if (m_sortOrder == sortOrder)
        return;

    m_sortOrder = sortOrder;
    Q_EMIT sortOrderChanged();
    invalidate();
}

/*!
    \qmlproperty int Sorter::priority

    This property holds the sort priority of this sorter.
    Sorters with a higher priority are applied first.
    In case of equal priority, Sorters are ordered by their insertion order.

    By default, the priority is 0.
*/
int Sorter::priority() const
{
    return m_priority;
}

void Sorter::setPriority(int priority)
{
    if (m_priority == priority)
        return;

    m_priority = priority;
    Q_EMIT priorityChanged();
    invalidate();
}

int Sorter::compareRows(const QModelIndex &source_left, const QModelIndex &source_right, const QQmlSortFilterProxyModel& proxyModel) const
{
    int comparison = compare(source_left, source_right, proxyModel);
    return (m_sortOrder == Qt::AscendingOrder) ? comparison : -comparison;
}

int Sorter::compare(const QModelIndex &sourceLeft, const QModelIndex &sourceRight, const QQmlSortFilterProxyModel& proxyModel) const
{
    if (lessThan(sourceLeft, sourceRight, proxyModel))
        return -1;
    if (lessThan(sourceRight, sourceLeft, proxyModel))
        return 1;
    return 0;
}

void Sorter::proxyModelCompleted(const QQmlSortFilterProxyModel& proxyModel)
{
    Q_UNUSED(proxyModel)
}

bool Sorter::lessThan(const QModelIndex &sourceLeft, const QModelIndex &sourceRight, const QQmlSortFilterProxyModel& proxyModel) const
{
    Q_UNUSED(sourceLeft)
    Q_UNUSED(sourceRight)
    Q_UNUSED(proxyModel)
    return false;
}

void Sorter::invalidate()
{
    if (m_enabled)
        Q_EMIT invalidated();
}

}
