#include "filtersorter.h"
#include "filters/filter.h"

namespace qqsfpm {

/*!
    \qmltype FilterSorter
    \inherits Sorter
    \inqmlmodule SortFilterProxyModel
    \ingroup Sorters
    \ingroup FilterContainer
    \brief Sorts rows based on if they match filters.

    A FilterSorter is a \l Sorter that orders row matching its filters before the rows not matching the filters.

    In the following example, rows with their \c favorite role set to \c true will be ordered at the beginning :
    \code
    SortFilterProxyModel {
        sourceModel: contactModel
        sorters: FilterSorter {
            ValueFilter { roleName: "favorite"; value: true }
        }
    }
    \endcode
    \sa FilterContainer
*/

/*!
    \qmlproperty list<Filter> FilterSorter::filters
    \default

    This property holds the list of filters for this filter sorter.
    If a row match all this FilterSorter's filters, it will be ordered before rows not matching all the filters.

    \sa Filter, FilterContainer
*/

int FilterSorter::compare(const QModelIndex& sourceLeft, const QModelIndex& sourceRight, const QQmlSortFilterProxyModel &proxyModel) const
{
    bool leftIsAccepted = indexIsAccepted(sourceLeft, proxyModel);
    bool rightIsAccepted = indexIsAccepted(sourceRight, proxyModel);

    if (leftIsAccepted == rightIsAccepted)
        return 0;

    return leftIsAccepted ? -1 : 1;
}

void FilterSorter::proxyModelCompleted(const QQmlSortFilterProxyModel& proxyModel)
{
    for (Filter* filter : m_filters)
        filter->proxyModelCompleted(proxyModel);
}

void FilterSorter::onFilterAppended(Filter* filter)
{
    connect(filter, &Filter::invalidated, this, &FilterSorter::invalidate);
    invalidate();
}

void FilterSorter::onFilterRemoved(Filter* filter)
{
    disconnect(filter, &Filter::invalidated, this, &FilterSorter::invalidate);
    invalidate();
}

void FilterSorter::onFiltersCleared()
{
    invalidate();
}

bool FilterSorter::indexIsAccepted(const QModelIndex& sourceIndex, const QQmlSortFilterProxyModel& proxyModel) const
{
    return std::all_of(m_filters.begin(), m_filters.end(),
        [&] (Filter* filter) {
            return filter->filterAcceptsRow(sourceIndex, proxyModel);
        }
    );
}

}
