#include "rolefilter.h"
#include "qqmlsortfilterproxymodel.h"

namespace qqsfpm {

/*!
    \qmltype RoleFilter
    \qmlabstract
    \inherits Filter
    \inqmlmodule SortFilterProxyModel
    \ingroup Filters
    \brief Base type for filters based on a source model role.

    The RoleFilter type cannot be used directly in a QML file.
    It exists to provide a set of common properties and methods,
    available across all the other filter types that inherit from it.
    Attempting to use the RoleFilter type directly will result in an error.
*/

/*!
    \qmlproperty string RoleFilter::roleName

    This property holds the role name that the filter is using to query the source model's data when filtering items.
*/
const QString& RoleFilter::roleName() const
{
    return m_roleName;
}

void RoleFilter::setRoleName(const QString& roleName)
{
    if (m_roleName == roleName)
        return;

    m_roleName = roleName;
    Q_EMIT roleNameChanged();
    invalidate();
}

QVariant RoleFilter::sourceData(const QModelIndex &sourceIndex, const QQmlSortFilterProxyModel& proxyModel) const
{
    return proxyModel.sourceData(sourceIndex, m_roleName);
}

}
