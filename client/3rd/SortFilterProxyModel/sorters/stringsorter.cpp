#include "stringsorter.h"

namespace qqsfpm {

/*!
    \qmltype StringSorter
    \inherits RoleSorter
    \inqmlmodule SortFilterProxyModel
    \ingroup Sorters
    \brief Sorts rows based on a source model string role.

    \l StringSorter is a specialized \l RoleSorter that sorts rows based on a source model string role.
    \l StringSorter compares strings according to a localized collation algorithm.

    In the following example, rows with be sorted by their \c lastName role :
    \code
    SortFilterProxyModel {
       sourceModel: contactModel
       sorters: StringSorter { roleName: "lastName" }
    }
    \endcode
*/

/*!
    \qmlproperty Qt.CaseSensitivity StringSorter::caseSensitivity

    This property holds the case sensitivity of the sorter.
*/
Qt::CaseSensitivity StringSorter::caseSensitivity() const
{
    return m_collator.caseSensitivity();
}

void StringSorter::setCaseSensitivity(Qt::CaseSensitivity caseSensitivity)
{
    if (m_collator.caseSensitivity() == caseSensitivity)
        return;

    m_collator.setCaseSensitivity(caseSensitivity);
    Q_EMIT caseSensitivityChanged();
    invalidate();
}

/*!
    \qmlproperty bool StringSorter::ignorePunctation

    This property holds whether the sorter ignores punctation.
    if \c ignorePunctuation is \c true, punctuation characters and symbols are ignored when determining sort order.

    \note This property is not currently supported on Apple platforms or if Qt is configured to not use ICU on Linux.
*/
bool StringSorter::ignorePunctation() const
{
    return m_collator.ignorePunctuation();
}

void StringSorter::setIgnorePunctation(bool ignorePunctation)
{
    if (m_collator.ignorePunctuation() == ignorePunctation)
        return;

    m_collator.setIgnorePunctuation(ignorePunctation);
    Q_EMIT ignorePunctationChanged();
    invalidate();
}

/*!
    \qmlproperty Locale StringSorter::locale

    This property holds the locale of the sorter.
*/
QLocale StringSorter::locale() const
{
    return m_collator.locale();
}

void StringSorter::setLocale(const QLocale &locale)
{
    if (m_collator.locale() == locale)
        return;

    m_collator.setLocale(locale);
    Q_EMIT localeChanged();
    invalidate();
}

/*!
    \qmlproperty bool StringSorter::numericMode

    This property holds whether the numeric mode of the sorter is enabled.
    This will enable proper sorting of numeric digits, so that e.g. 100 sorts after 99.
    By default this mode is off.
*/
bool StringSorter::numericMode() const
{
    return m_collator.numericMode();
}

void StringSorter::setNumericMode(bool numericMode)
{
    if (m_collator.numericMode() == numericMode)
        return;

    m_collator.setNumericMode(numericMode);
    Q_EMIT numericModeChanged();
    invalidate();
}

int StringSorter::compare(const QModelIndex &sourceLeft, const QModelIndex &sourceRight, const QQmlSortFilterProxyModel& proxyModel) const
{
    QPair<QVariant, QVariant> pair = sourceData(sourceLeft, sourceRight, proxyModel);
    QString leftValue = pair.first.toString();
    QString rightValue = pair.second.toString();
    return m_collator.compare(leftValue, rightValue);
}

}
