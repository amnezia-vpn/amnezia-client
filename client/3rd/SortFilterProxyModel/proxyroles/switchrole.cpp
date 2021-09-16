#include "switchrole.h"
#include "qqmlsortfilterproxymodel.h"
#include "filters/filter.h"
#include <QtQml>

namespace qqsfpm {

/*!
    \qmltype SwitchRole
    \inherits SingleRole
    \inqmlmodule SortFilterProxyModel
    \ingroup ProxyRoles
    \ingroup FilterContainer
    \brief A role using \l Filter to conditionnaly compute its data.

    A SwitchRole is a \l ProxyRole that computes its data with the help of \l Filter.
    Each top level filters specified in the \l SwitchRole is evaluated on the rows of the model, if a \l Filter evaluates to true, the data of the \l SwitchRole for this row will be the one of the attached \l {value} {SwitchRole.value} property.
    If no top level filters evaluate to true, the data will default to the one of the \l defaultRoleName (or the \l defaultValue if no \l defaultRoleName is specified).

    In the following example, the \c favoriteOrFirstNameSection role is equal to \c * if the \c favorite role of a row is true, otherwise it's the same as the \c firstName role :
    \code
    SortFilterProxyModel {
       sourceModel: contactModel
       proxyRoles: SwitchRole {
           name: "favoriteOrFirstNameSection"
           filters: ValueFilter {
               roleName: "favorite"
               value: true
               SwitchRole.value: "*"
           }
           defaultRoleName: "firstName"
        }
    }
    \endcode
    \sa FilterContainer
*/
SwitchRoleAttached::SwitchRoleAttached(QObject* parent) : QObject (parent)
{
    if (!qobject_cast<Filter*>(parent))
        qmlInfo(parent) << "SwitchRole must be attached to a Filter";
}

/*!
    \qmlattachedproperty var SwitchRole::value

    This property attaches a value to a \l Filter.
*/
QVariant SwitchRoleAttached::value() const
{
    return m_value;
}

void SwitchRoleAttached::setValue(QVariant value)
{
    if (m_value == value)
        return;

    m_value = value;
    Q_EMIT valueChanged();
}

/*!
    \qmlproperty string SwitchRole::defaultRoleName

    This property holds the default role name of the role.
    If no filter match a row, the data of this role will be the data of the role whose name is \c defaultRoleName.
*/
QString SwitchRole::defaultRoleName() const
{
    return m_defaultRoleName;
}

void SwitchRole::setDefaultRoleName(const QString& defaultRoleName)
{
    if (m_defaultRoleName == defaultRoleName)
        return;

    m_defaultRoleName = defaultRoleName;
    Q_EMIT defaultRoleNameChanged();
    invalidate();
}

/*!
    \qmlproperty var SwitchRole::defaultValue

    This property holds the default value of the role.
    If no filter match a row, and no \l defaultRoleName is set, the data of this role will be \c defaultValue.
*/
QVariant SwitchRole::defaultValue() const
{
    return m_defaultValue;
}

void SwitchRole::setDefaultValue(const QVariant& defaultValue)
{
    if (m_defaultValue == defaultValue)
        return;

    m_defaultValue = defaultValue;
    Q_EMIT defaultValueChanged();
    invalidate();
}

/*!
    \qmlproperty list<Filter> SwitchRole::filters
    \default

    This property holds the list of filters for this proxy role.
    The data of this role will be equal to the attached \l {value} {SwitchRole.value} property of the first filter that matches the model row.

    \sa Filter, FilterContainer
*/

void SwitchRole::proxyModelCompleted(const QQmlSortFilterProxyModel& proxyModel)
{
    for (Filter* filter : m_filters)
        filter->proxyModelCompleted(proxyModel);
}

SwitchRoleAttached* SwitchRole::qmlAttachedProperties(QObject* object)
{
    return new SwitchRoleAttached(object);
}

QVariant SwitchRole::data(const QModelIndex &sourceIndex, const QQmlSortFilterProxyModel &proxyModel)
{
    for (auto filter: m_filters) {
        if (!filter->enabled())
            continue;
        if (filter->filterAcceptsRow(sourceIndex, proxyModel)) {
            auto attached = static_cast<SwitchRoleAttached*>(qmlAttachedPropertiesObject<SwitchRole>(filter, false));
            if (!attached) {
                qWarning() << "No SwitchRole.value provided for this filter" << filter;
                continue;
            }
            QVariant value = attached->value();
            if (!value.isValid()) {
                qWarning() << "No SwitchRole.value provided for this filter" << filter;
                continue;
            }
            return value;
        }
    }
    if (!m_defaultRoleName.isEmpty())
        return proxyModel.sourceData(sourceIndex, m_defaultRoleName);
    return m_defaultValue;
}

void SwitchRole::onFilterAppended(Filter *filter)
{
    connect(filter, &Filter::invalidated, this, &SwitchRole::invalidate);
    auto attached = static_cast<SwitchRoleAttached*>(qmlAttachedPropertiesObject<SwitchRole>(filter, true));
    connect(attached, &SwitchRoleAttached::valueChanged, this, &SwitchRole::invalidate);
    invalidate();
}

void SwitchRole::onFilterRemoved(Filter *filter)
{
    Q_UNUSED(filter)
    invalidate();
}

void SwitchRole::onFiltersCleared()
{
    invalidate();
}

}
