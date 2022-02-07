#include "filter.h"
#include "qqmlsortfilterproxymodel.h"

namespace qqsfpm {

/*!
    \qmltype Filter
    \qmlabstract
    \inqmlmodule SortFilterProxyModel
    \ingroup Filters
    \brief Base type for the \l SortFilterProxyModel filters.

    The Filter type cannot be used directly in a QML file.
    It exists to provide a set of common properties and methods,
    available across all the other filter types that inherit from it.
    Attempting to use the Filter type directly will result in an error.
*/

Filter::Filter(QObject *parent) : QObject(parent)
{
}

/*!
    \qmlproperty bool Filter::enabled

    This property holds whether the filter is enabled.
    A disabled filter will accept every rows unconditionally (even if it's inverted).

    By default, filters are enabled.
*/
bool Filter::enabled() const
{
    return m_enabled;
}

void Filter::setEnabled(bool enabled)
{
    if (m_enabled == enabled)
        return;

    m_enabled = enabled;
    Q_EMIT enabledChanged();
    Q_EMIT invalidated();
}

/*!
    \qmlproperty bool Filter::inverted

    This property holds whether the filter is inverted.
    When a filter is inverted, a row normally accepted would be rejected, and vice-versa.

    By default, filters are not inverted.
*/
bool Filter::inverted() const
{
    return m_inverted;
}

void Filter::setInverted(bool inverted)
{
    if (m_inverted == inverted)
        return;

    m_inverted = inverted;
    Q_EMIT invertedChanged();
    invalidate();
}

bool Filter::filterAcceptsRow(const QModelIndex &sourceIndex, const QQmlSortFilterProxyModel& proxyModel) const
{
    return !m_enabled || filterRow(sourceIndex, proxyModel) ^ m_inverted;
}

void Filter::proxyModelCompleted(const QQmlSortFilterProxyModel& proxyModel)
{
    Q_UNUSED(proxyModel)
}

void Filter::invalidate()
{
    if (m_enabled)
        Q_EMIT invalidated();
}

}
