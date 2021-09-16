#include "filterrole.h"
#include "filters/filter.h"

namespace qqsfpm {

/*!
    \qmltype FilterRole
    \inherits SingleRole
    \inqmlmodule SortFilterProxyModel
    \ingroup ProxyRoles
    \ingroup FilterContainer
    \brief A role resolving to \c true for rows matching all its filters.

    A FilterRole is a \l ProxyRole that returns \c true for rows matching all its filters.

    In the following example, the \c isAdult role will be equal to \c true if the \c age role is superior or equal to 18.
    \code
    SortFilterProxyModel {
        sourceModel: personModel
        proxyRoles: FilterRole {
            name: "isAdult"
            RangeFilter { roleName: "age"; minimumValue: 18; minimumInclusive: true }
        }
    }
    \endcode
    \sa FilterContainer
*/

/*!
    \qmlproperty list<Filter> FilterRole::filters
    \default

    This property holds the list of filters for this filter role.
    The data of this role will be equal to the \c true if all its filters match the model row, \c false otherwise.

    \sa Filter, FilterContainer
*/

void FilterRole::onFilterAppended(Filter* filter)
{
    connect(filter, &Filter::invalidated, this, &FilterRole::invalidate);
    invalidate();
}

void FilterRole::onFilterRemoved(Filter* filter)
{
    disconnect(filter, &Filter::invalidated, this, &FilterRole::invalidate);
    invalidate();
}

void FilterRole::onFiltersCleared()
{
    invalidate();
}

QVariant FilterRole::data(const QModelIndex& sourceIndex, const QQmlSortFilterProxyModel& proxyModel)
{
    return std::all_of(m_filters.begin(), m_filters.end(),
        [&] (Filter* filter) {
            return filter->filterAcceptsRow(sourceIndex, proxyModel);
        }
    );
}

}
