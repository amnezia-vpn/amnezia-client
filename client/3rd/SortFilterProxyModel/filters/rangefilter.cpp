#include "rangefilter.h"
#include "../utils/utils.h"

namespace qqsfpm {

/*!
    \qmltype RangeFilter
    \inherits RoleFilter
    \inqmlmodule SortFilterProxyModel
    \ingroup Filters
    \brief Filters rows between boundary values.

    A RangeFilter is a \l RoleFilter that accepts rows if their data is between the filter's minimum and maximum value.

    In the following example, only rows with their \c price role set to a value between the tow boundary of the slider will be accepted :
    \code
    RangeSlider {
       id: priceRangeSlider
    }

    SortFilterProxyModel {
       sourceModel: priceModel
       filters: RangeFilter {
           roleName: "price"
           minimumValue: priceRangeSlider.first.value
           maximumValue: priceRangeSlider.second.value
       }
    }
    \endcode
*/

/*!
    \qmlproperty int RangeFilter::minimumValue

    This property holds the minimumValue of the filter.
    Rows with a value lower than \c minimumValue will be rejected.

    By default, no value is set.

    \sa minimumInclusive
*/
QVariant RangeFilter::minimumValue() const
{
    return m_minimumValue;
}

void RangeFilter::setMinimumValue(QVariant minimumValue)
{
    if (m_minimumValue == minimumValue)
        return;

    m_minimumValue = minimumValue;
    Q_EMIT minimumValueChanged();
    invalidate();
}

/*!
    \qmlproperty int RangeFilter::minimumInclusive

    This property holds whether the \l minimumValue is inclusive.

    By default, the \l minimumValue is inclusive.

    \sa minimumValue
*/
bool RangeFilter::minimumInclusive() const
{
    return m_minimumInclusive;
}

void RangeFilter::setMinimumInclusive(bool minimumInclusive)
{
    if (m_minimumInclusive == minimumInclusive)
        return;

    m_minimumInclusive = minimumInclusive;
    Q_EMIT minimumInclusiveChanged();
    invalidate();
}

/*!
    \qmlproperty int RangeFilter::maximumValue

    This property holds the maximumValue of the filter.
    Rows with a value higher than \c maximumValue will be rejected.

    By default, no value is set.

    \sa maximumInclusive
*/
QVariant RangeFilter::maximumValue() const
{
    return m_maximumValue;
}

void RangeFilter::setMaximumValue(QVariant maximumValue)
{
    if (m_maximumValue == maximumValue)
        return;

    m_maximumValue = maximumValue;
    Q_EMIT maximumValueChanged();
    invalidate();
}

/*!
    \qmlproperty int RangeFilter::maximumInclusive

    This property holds whether the \l minimumValue is inclusive.

    By default, the \l minimumValue is inclusive.

    \sa minimumValue
*/
bool RangeFilter::maximumInclusive() const
{
    return m_maximumInclusive;
}

void RangeFilter::setMaximumInclusive(bool maximumInclusive)
{
    if (m_maximumInclusive == maximumInclusive)
        return;

    m_maximumInclusive = maximumInclusive;
    Q_EMIT maximumInclusiveChanged();
    invalidate();
}

bool RangeFilter::filterRow(const QModelIndex& sourceIndex, const QQmlSortFilterProxyModel& proxyModel) const
{
    const QVariant value = sourceData(sourceIndex, proxyModel);
    bool lessThanMin = m_minimumValue.isValid() &&
            (m_minimumInclusive ? value < m_minimumValue : value <= m_minimumValue);
    bool moreThanMax = m_maximumValue.isValid() &&
            (m_maximumInclusive ? value > m_maximumValue : value >= m_maximumValue);
    return !(lessThanMin || moreThanMax);
}

}
