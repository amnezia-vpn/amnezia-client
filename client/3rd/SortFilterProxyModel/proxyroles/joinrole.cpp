#include "joinrole.h"
#include "qqmlsortfilterproxymodel.h"

namespace qqsfpm {

/*!
    \qmltype JoinRole
    \inherits SingleRole
    \inqmlmodule SortFilterProxyModel
    \ingroup ProxyRoles
    \brief a role made from concatenating other roles.

    A JoinRole is a simple \l ProxyRole that concatenates other roles.

    In the following example, the \c fullName role is computed by the concatenation of the \c firstName role and the \c lastName role separated by a space :
    \code
    SortFilterProxyModel {
       sourceModel: contactModel
       proxyRoles: JoinRole {
           name: "fullName"
           roleNames: ["firstName", "lastName"]
      }
    }
    \endcode

*/

/*!
    \qmlproperty list<string> JoinRole::roleNames

    This property holds the role names that are joined by this role.
*/
QStringList JoinRole::roleNames() const
{
    return m_roleNames;
}

void JoinRole::setRoleNames(const QStringList& roleNames)
{
    if (m_roleNames == roleNames)
        return;

    m_roleNames = roleNames;
    Q_EMIT roleNamesChanged();
    invalidate();
}

/*!
    \qmlproperty string JoinRole::separator

    This property holds the separator that is used to join the roles specified in \l roleNames.

    By default, it's a space.
*/
QString JoinRole::separator() const
{
    return m_separator;
}

void JoinRole::setSeparator(const QString& separator)
{
    if (m_separator == separator)
        return;

    m_separator = separator;
    Q_EMIT separatorChanged();
    invalidate();
}

QVariant JoinRole::data(const QModelIndex &sourceIndex, const QQmlSortFilterProxyModel& proxyModel)
{
    QString result;

    for (const QString& roleName : m_roleNames)
        result += proxyModel.sourceData(sourceIndex, roleName).toString() + m_separator;

    if (!m_roleNames.isEmpty())
        result.chop(m_separator.length());

    return result;
}

}
