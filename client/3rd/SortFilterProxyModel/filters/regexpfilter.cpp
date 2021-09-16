#include "regexpfilter.h"
#include <QVariant>

namespace qqsfpm {

/*!
    \qmltype RegExpFilter
    \inherits RoleFilter
    \inqmlmodule SortFilterProxyModel
    \ingroup Filters
    \brief  Filters rows matching a regular expression.

    A RegExpFilter is a \l RoleFilter that accepts rows matching a regular rexpression.

    In the following example, only rows with their \c lastName role beggining with the content of textfield the will be accepted:
    \code
    TextField {
       id: nameTextField
    }

    SortFilterProxyModel {
       sourceModel: contactModel
       filters: RegExpFilter {
           roleName: "lastName"
           pattern: "^" + nameTextField.displayText
       }
    }
    \endcode
*/

/*!
    \qmlproperty bool RegExpFilter::pattern

    The pattern used to filter the contents of the source model.

    \sa syntax
*/
QString RegExpFilter::pattern() const
{
    return m_pattern;
}

void RegExpFilter::setPattern(const QString& pattern)
{
    if (m_pattern == pattern)
        return;

    m_pattern = pattern;
    m_regExp.setPattern(pattern);
    Q_EMIT patternChanged();
    invalidate();
}

/*!
    \qmlproperty enum RegExpFilter::syntax

    The pattern used to filter the contents of the source model.

    Only the source model's value having their \l RoleFilter::roleName data matching this \l pattern with the specified \l syntax will be kept.

    \value RegExpFilter.RegExp A rich Perl-like pattern matching syntax. This is the default.
    \value RegExpFilter.Wildcard This provides a simple pattern matching syntax similar to that used by shells (command interpreters) for "file globbing".
    \value RegExpFilter.FixedString The pattern is a fixed string. This is equivalent to using the RegExp pattern on a string in which all metacharacters are escaped.
    \value RegExpFilter.RegExp2 Like RegExp, but with greedy quantifiers.
    \value RegExpFilter.WildcardUnix This is similar to Wildcard but with the behavior of a Unix shell. The wildcard characters can be escaped with the character "\".
    \value RegExpFilter.W3CXmlSchema11 The pattern is a regular expression as defined by the W3C XML Schema 1.1 specification.

    \sa pattern
*/
RegExpFilter::PatternSyntax RegExpFilter::syntax() const
{
    return m_syntax;
}

void RegExpFilter::setSyntax(RegExpFilter::PatternSyntax syntax)
{
    if (m_syntax == syntax)
        return;

    m_syntax = syntax;
    m_regExp.setPatternSyntax(static_cast<QRegExp::PatternSyntax>(syntax));
    Q_EMIT syntaxChanged();
    invalidate();
}

/*!
    \qmlproperty Qt::CaseSensitivity RegExpFilter::caseSensitivity

    This property holds the caseSensitivity of the filter.
*/
Qt::CaseSensitivity RegExpFilter::caseSensitivity() const
{
    return m_caseSensitivity;
}

void RegExpFilter::setCaseSensitivity(Qt::CaseSensitivity caseSensitivity)
{
    if (m_caseSensitivity == caseSensitivity)
        return;

    m_caseSensitivity = caseSensitivity;
    m_regExp.setCaseSensitivity(caseSensitivity);
    Q_EMIT caseSensitivityChanged();
    invalidate();
}

bool RegExpFilter::filterRow(const QModelIndex& sourceIndex, const QQmlSortFilterProxyModel& proxyModel) const
{
    QString string = sourceData(sourceIndex, proxyModel).toString();
    return m_regExp.indexIn(string) != -1;
}

}
