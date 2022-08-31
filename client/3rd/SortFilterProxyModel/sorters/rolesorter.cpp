#include "rolesorter.h"
#include "qqmlsortfilterproxymodel.h"
#include "../utils/utils.h"

namespace qqsfpm {

/*!
    \qmltype RoleSorter
    \inherits Sorter
    \inqmlmodule SortFilterProxyModel
    \ingroup Sorters
    \brief Sorts rows based on a source model role.

    A RoleSorter is a simple \l Sorter that sorts rows based on a source model role.

    In the following example, rows with be sorted by their \c lastName role :
    \code
    SortFilterProxyModel {
       sourceModel: contactModel
       sorters: RoleSorter { roleName: "lastName" }
    }
    \endcode
*/

/*!
    \qmlproperty string RoleSorter::roleName

    This property holds the role name that the sorter is using to query the source model's data when sorting items.
*/
const QString& RoleSorter::roleName() const
{
    return m_roleName;
}

void RoleSorter::setRoleName(const QString& roleName)
{
    if (m_roleName == roleName)
        return;

    m_roleName = roleName;
    Q_EMIT roleNameChanged();
    invalidate();
}

QPair<QVariant, QVariant> RoleSorter::sourceData(const QModelIndex &sourceLeft, const QModelIndex& sourceRight, const QQmlSortFilterProxyModel& proxyModel) const
{
    QPair<QVariant, QVariant> pair;
    int role = proxyModel.roleForName(m_roleName);

    if (role == -1)
        return pair;

    pair.first = proxyModel.sourceData(sourceLeft, role);
    pair.second = proxyModel.sourceData(sourceRight, role);
    return pair;
}

int RoleSorter::compare(const QModelIndex &sourceLeft, const QModelIndex& sourceRight, const QQmlSortFilterProxyModel& proxyModel) const
{
    const QPair<QVariant, QVariant> pair = sourceData(sourceLeft, sourceRight, proxyModel);
    return compareVariants(pair.first, pair.second);
}

}
