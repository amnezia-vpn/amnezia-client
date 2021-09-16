#include "singlerole.h"
#include <QVariant>

namespace qqsfpm {

/*!
    \qmltype SingleRole
    \qmlabstract
    \inherits ProxyRole
    \inqmlmodule SortFilterProxyModel
    \ingroup ProxyRoles
    \brief Base type for the \l SortFilterProxyModel proxy roles defining a single role.

    SingleRole is a convenience base class for proxy roles who define a single role.
    It cannot be used directly in a QML file.
    It exists to provide a set of common properties and methods,
    available across all the other proxy role types that inherit from it.
    Attempting to use the SingleRole type directly will result in an error.
*/
/*!
    \qmlproperty string SingleRole::name

    This property holds the role name of the proxy role.
*/
QString SingleRole::name() const
{
    return m_name;
}

void SingleRole::setName(const QString& name)
{
    if (m_name == name)
        return;

    Q_EMIT namesAboutToBeChanged();
    m_name = name;
    Q_EMIT nameChanged();
    Q_EMIT namesChanged();
}

QStringList SingleRole::names()
{
    return QStringList { m_name };
}

QVariant SingleRole::data(const QModelIndex &sourceIndex, const QQmlSortFilterProxyModel &proxyModel, const QString &name)
{
    Q_UNUSED(name);
    return data(sourceIndex, proxyModel);
}

}
