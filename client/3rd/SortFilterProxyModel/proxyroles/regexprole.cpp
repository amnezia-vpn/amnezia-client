#include "regexprole.h"
#include "qqmlsortfilterproxymodel.h"
#include <QDebug>

namespace qqsfpm {

/*!
    \qmltype RegExpRole
    \inherits ProxyRole
    \inqmlmodule SortFilterProxyModel
    \ingroup ProxyRoles
    \brief A ProxyRole extracting data from a source role via a regular expression.

    A RegExpRole is a \l ProxyRole that provides a role for each named capture group of its regular expression \l pattern.

    In the following example, the \c date role of the source model will be extracted in 3 roles in the proxy moodel: \c year, \c month and \c day.
    \code
    SortFilterProxyModel {
        sourceModel: eventModel
        proxyRoles: RegExpRole {
            roleName: "date"
            pattern: "(?<year>\\d{4})-(?<month>\\d{2})-(?<day>\\d{2})"
        }
    }
    \endcode
*/

/*!
    \qmlproperty QString RegExpRole::roleName

    This property holds the role name that the RegExpRole is using to query the source model's data to extract new roles from.
*/
QString RegExpRole::roleName() const
{
    return m_roleName;
}

void RegExpRole::setRoleName(const QString& roleName)
{
    if (m_roleName == roleName)
        return;

    m_roleName = roleName;
    Q_EMIT roleNameChanged();
}

/*!
    \qmlproperty QString RegExpRole::pattern

    This property holds the pattern of the regular expression of this RegExpRole.
    The RegExpRole will expose a role for each of the named capture group of the pattern.
*/
QString RegExpRole::pattern() const
{
    return m_regularExpression.pattern();
}

void RegExpRole::setPattern(const QString& pattern)
{
    if (m_regularExpression.pattern() == pattern)
        return;

    Q_EMIT namesAboutToBeChanged();
    m_regularExpression.setPattern(pattern);
    invalidate();
    Q_EMIT patternChanged();
    Q_EMIT namesChanged();
}

/*!
    \qmlproperty Qt::CaseSensitivity RegExpRole::caseSensitivity

    This property holds the caseSensitivity of the regular expression.
*/
Qt::CaseSensitivity RegExpRole::caseSensitivity() const
{
    return m_regularExpression.patternOptions() & QRegularExpression::CaseInsensitiveOption ?
                Qt::CaseInsensitive : Qt::CaseSensitive;
}

void RegExpRole::setCaseSensitivity(Qt::CaseSensitivity caseSensitivity)
{
    if (this->caseSensitivity() == caseSensitivity)
        return;

    m_regularExpression.setPatternOptions(m_regularExpression.patternOptions() ^ QRegularExpression::CaseInsensitiveOption); //toggle the option
    Q_EMIT caseSensitivityChanged();
}

QStringList RegExpRole::names()
{
    QStringList nameCaptureGroups = m_regularExpression.namedCaptureGroups();
    nameCaptureGroups.removeAll("");
    return nameCaptureGroups;
}

QVariant RegExpRole::data(const QModelIndex& sourceIndex, const QQmlSortFilterProxyModel& proxyModel, const QString &name)
{
    QString text = proxyModel.sourceData(sourceIndex, m_roleName).toString();
    QRegularExpressionMatch match = m_regularExpression.match(text);
    return match.hasMatch() ? (match.captured(name)) : QVariant{};
}

}
