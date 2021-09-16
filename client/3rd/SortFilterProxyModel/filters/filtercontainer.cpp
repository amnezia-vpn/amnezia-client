#include "filtercontainer.h"
#include "filter.h"
#include <QtQml>

namespace qqsfpm {

/*!
    \qmltype FilterContainer
    \qmlabstract
    \inqmlmodule SortFilterProxyModel
    \ingroup FilterAttached
    \brief Abstract interface for types containing \l {Filter}{Filters}.

    \section2 Types implementing this interface:
    \annotatedlist FilterContainer
*/

QList<Filter*> FilterContainer::filters() const
{
    return m_filters;
}

void FilterContainer::appendFilter(Filter* filter)
{
    m_filters.append(filter);
    onFilterAppended(filter);
}

void FilterContainer::removeFilter(Filter* filter)
{
    m_filters.removeOne(filter);
    onFilterRemoved(filter);
}

void FilterContainer::clearFilters()
{
    m_filters.clear();
    onFiltersCleared();
}

QQmlListProperty<Filter> FilterContainer::filtersListProperty()
{
    return QQmlListProperty<Filter>(reinterpret_cast<QObject*>(this), &m_filters,
                                    &FilterContainer::append_filter,
                                    &FilterContainer::count_filter,
                                    &FilterContainer::at_filter,
                                    &FilterContainer::clear_filters);
}

void FilterContainer::append_filter(QQmlListProperty<Filter>* list, Filter* filter)
{
    if (!filter)
        return;

    FilterContainer* that = reinterpret_cast<FilterContainer*>(list->object);
    that->appendFilter(filter);
}

int FilterContainer::count_filter(QQmlListProperty<Filter>* list)
{
    QList<Filter*>* filters = static_cast<QList<Filter*>*>(list->data);
    return filters->count();
}

Filter* FilterContainer::at_filter(QQmlListProperty<Filter>* list, int index)
{
    QList<Filter*>* filters = static_cast<QList<Filter*>*>(list->data);
    return filters->at(index);
}

void FilterContainer::clear_filters(QQmlListProperty<Filter> *list)
{
    FilterContainer* that = reinterpret_cast<FilterContainer*>(list->object);
    that->clearFilters();
}

FilterContainerAttached::FilterContainerAttached(QObject* object) : QObject(object),
    m_filter(qobject_cast<Filter*>(object))
{
    if (!m_filter)
        qmlWarning(object) << "FilterContainer must be attached to a Filter";
}

FilterContainerAttached::~FilterContainerAttached()
{
    if (m_filter && m_container) {
        FilterContainer* container = qobject_cast<FilterContainer*>(m_container.data());
        container->removeFilter(m_filter);
    }
}

/*!
    \qmlattachedproperty bool FilterContainer::container
    This attached property allows you to include in a \l FilterContainer a \l Filter that
    has been instantiated outside of the \l FilterContainer, for example in an Instantiator.
*/
QObject* FilterContainerAttached::container() const
{
    return m_container;
}

void FilterContainerAttached::setContainer(QObject* object)
{
    if (m_container == object)
        return;

    FilterContainer* container = qobject_cast<FilterContainer*>(object);
    if (object && !container)
        qmlWarning(parent()) << "container must inherits from FilterContainer, " << object->metaObject()->className() << " provided";

    if (m_container && m_filter)
        qobject_cast<FilterContainer*>(m_container.data())->removeFilter(m_filter);

    m_container = container ? object : nullptr;
    if (container && m_filter)
        container->appendFilter(m_filter);

    Q_EMIT containerChanged();
}

FilterContainerAttached* FilterContainerAttached::qmlAttachedProperties(QObject* object)
{
    return new FilterContainerAttached(object);
}

}
