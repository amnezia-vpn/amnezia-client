#include "alloffilter.h"

namespace qqsfpm {

/*!
    \qmltype AllOf
    \inherits Filter
    \inqmlmodule SortFilterProxyModel
    \ingroup Filters
    \ingroup FilterContainer
    \brief Filter container accepting rows accepted by all its child filters.

    The AllOf type is a \l Filter container that accepts rows if all of its contained (and enabled) filters accept them, or if it has no filter.

    Using it as a top level filter has the same effect as putting all its child filters as top level filters. It can however be usefull to use an AllOf filter when nested in an AnyOf filter.
    \sa FilterContainer
*/
bool AllOfFilter::filterRow(const QModelIndex& sourceIndex, const QQmlSortFilterProxyModel& proxyModel) const
{
    //return true if all filters return false, or if there is no filter.
    return std::all_of(m_filters.begin(), m_filters.end(),
        [&sourceIndex, &proxyModel] (Filter* filter) {
            return filter->filterAcceptsRow(sourceIndex, proxyModel);
        }
    );
}

}
