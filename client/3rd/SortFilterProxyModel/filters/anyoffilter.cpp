#include "anyoffilter.h"

namespace qqsfpm {

/*!
    \qmltype AnyOf
    \inherits Filter
    \inqmlmodule SortFilterProxyModel
    \ingroup Filters
    \ingroup FilterContainer
    \brief Filter container accepting rows accepted by at least one of its child filters.

    The AnyOf type is a \l Filter container that accepts rows if any of its contained (and enabled) filters accept them.

    In the following example, only the rows where the \c firstName role or the \c lastName role match the text entered in the \c nameTextField will be accepted :
    \code
    TextField {
      id: nameTextField
    }

    SortFilterProxyModel {
      sourceModel: contactModel
      filters: AnyOf {
          RegExpFilter {
              roleName: "lastName"
              pattern: nameTextField.text
              caseSensitivity: Qt.CaseInsensitive
          }
          RegExpFilter {
              roleName: "firstName"
              pattern: nameTextField.text
              caseSensitivity: Qt.CaseInsensitive
          }
      }
    }
    \endcode
    \sa FilterContainer
*/
bool AnyOfFilter::filterRow(const QModelIndex& sourceIndex, const QQmlSortFilterProxyModel& proxyModel) const
{
    //return true if any of the enabled filters return true
    return std::any_of(m_filters.begin(), m_filters.end(),
        [&sourceIndex, &proxyModel] (Filter* filter) {
            return filter->enabled() && filter->filterAcceptsRow(sourceIndex, proxyModel);
        }
    );
}

}
