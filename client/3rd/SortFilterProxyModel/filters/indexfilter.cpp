#include "indexfilter.h"
#include "qqmlsortfilterproxymodel.h"

namespace qqsfpm {

/*!
    \qmltype IndexFilter
    \inherits Filter
    \inqmlmodule SortFilterProxyModel
    \ingroup Filters
    \brief Filters rows based on their source index.

    An IndexFilter is a filter allowing contents to be filtered based on their source model index.

    In the following example, only the first row of the source model will be accepted:
    \code
    SortFilterProxyModel {
       sourceModel: contactModel
       filters: IndexFilter {
           maximumIndex: 0
       }
    }
    \endcode
*/

/*!
    \qmlproperty int IndexFilter::minimumIndex

    This property holds the minimumIndex of the filter.
    Rows with a source index lower than \c minimumIndex will be rejected.

    If \c minimumIndex is negative, it is counted from the end of the source model, meaning that :
    \code
    minimumIndex: -1
    \endcode
    is equivalent to :
    \code
    minimumIndex: sourceModel.count - 1
    \endcode
    By default, no value is set.
*/
const QVariant& IndexFilter::minimumIndex() const
{
    return m_minimumIndex;
}

void IndexFilter::setMinimumIndex(const QVariant& minimumIndex)
{
    if (m_minimumIndex == minimumIndex)
        return;

    m_minimumIndex = minimumIndex;
    Q_EMIT minimumIndexChanged();
    invalidate();
}

/*!
    \qmlproperty int IndexFilter::maximumIndex

    This property holds the maximumIndex of the filter.
    Rows with a source index higher than \c maximumIndex will be rejected.

    If \c maximumIndex is negative, it is counted from the end of the source model, meaning that:
    \code
    maximumIndex: -1
    \endcode
    is equivalent to :
    \code
    maximumIndex: sourceModel.count - 1
    \endcode
    By default, no value is set.
*/
const QVariant& IndexFilter::maximumIndex() const
{
    return m_maximumIndex;
}

void IndexFilter::setMaximumIndex(const QVariant& maximumIndex)
{
    if (m_maximumIndex == maximumIndex)
        return;

    m_maximumIndex = maximumIndex;
    Q_EMIT maximumIndexChanged();
    invalidate();
}

bool IndexFilter::filterRow(const QModelIndex& sourceIndex, const QQmlSortFilterProxyModel& proxyModel) const
{
    int sourceRowCount = proxyModel.sourceModel()->rowCount();
    int sourceRow = sourceIndex.row();

    bool minimumIsValid;
    int minimum = m_minimumIndex.toInt(&minimumIsValid);
    if (minimumIsValid) {
        int actualMinimum = minimum < 0 ? sourceRowCount + minimum : minimum;
        if (sourceRow < actualMinimum)
            return false;
    }

    bool maximumIsValid;
    int maximum = m_maximumIndex.toInt(&maximumIsValid);
    if (maximumIsValid) {
        int actualMaximum = maximum < 0 ? sourceRowCount + maximum : maximum;
        if (sourceRow > actualMaximum)
            return false;
    }

    return true;
}

}
