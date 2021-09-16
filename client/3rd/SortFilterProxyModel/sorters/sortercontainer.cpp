#include "sortercontainer.h"
#include "sorter.h"
#include <QtQml>

namespace qqsfpm {

/*!
    \qmltype SorterContainer
    \qmlabstract
    \inqmlmodule SortFilterProxyModel
    \ingroup SorterAttached
    \brief Abstract interface for types containing \l {Sorter}{Sorters}.

    \section2 Types implementing this interface:
    \annotatedlist SorterContainer
*/

QList<Sorter*> SorterContainer::sorters() const
{
    return m_sorters;
}

void SorterContainer::appendSorter(Sorter* sorter)
{
    m_sorters.append(sorter);
    onSorterAppended(sorter);
}

void SorterContainer::removeSorter(Sorter *sorter)
{
    m_sorters.removeOne(sorter);
    onSorterRemoved(sorter);
}

void SorterContainer::clearSorters()
{
    m_sorters.clear();
    onSortersCleared();
}

QQmlListProperty<Sorter> SorterContainer::sortersListProperty()
{
    return QQmlListProperty<Sorter>(reinterpret_cast<QObject*>(this), &m_sorters,
                                    &SorterContainer::append_sorter,
                                    &SorterContainer::count_sorter,
                                    &SorterContainer::at_sorter,
                                    &SorterContainer::clear_sorters);
}

void SorterContainer::append_sorter(QQmlListProperty<Sorter>* list, Sorter* sorter)
{
    if (!sorter)
        return;

    SorterContainer* that = reinterpret_cast<SorterContainer*>(list->object);
    that->appendSorter(sorter);
}

int SorterContainer::count_sorter(QQmlListProperty<Sorter>* list)
{
    QList<Sorter*>* sorters = static_cast<QList<Sorter*>*>(list->data);
    return sorters->count();
}

Sorter* SorterContainer::at_sorter(QQmlListProperty<Sorter>* list, int index)
{
    QList<Sorter*>* sorters = static_cast<QList<Sorter*>*>(list->data);
    return sorters->at(index);
}

void SorterContainer::clear_sorters(QQmlListProperty<Sorter> *list)
{
    SorterContainer* that = reinterpret_cast<SorterContainer*>(list->object);
    that->clearSorters();
}

SorterContainerAttached::SorterContainerAttached(QObject* object) : QObject(object),
    m_sorter(qobject_cast<Sorter*>(object))
{
    if (!m_sorter)
        qmlWarning(object) << "SorterContainerAttached must be attached to a Sorter";
}

SorterContainerAttached::~SorterContainerAttached()
{
    if (m_sorter && m_container) {
        SorterContainer* container = qobject_cast<SorterContainer*>(m_container.data());
        container->removeSorter(m_sorter);
    }
}

/*!
    \qmlattachedproperty bool SorterContainer::container
    This attached property allows you to include in a \l SorterContainer a \l Sorter that
    has been instantiated outside of the \l SorterContainer, for example in an Instantiator.
*/
QObject* SorterContainerAttached::container() const
{
    return m_container;
}

void SorterContainerAttached::setContainer(QObject* object)
{
    if (m_container == object)
        return;

    SorterContainer* container = qobject_cast<SorterContainer*>(object);
    if (object && !container)
        qmlWarning(parent()) << "container must inherits from SorterContainer, " << object->metaObject()->className() << " provided";

    if (m_container && m_sorter)
        qobject_cast<SorterContainer*>(m_container.data())->removeSorter(m_sorter);

    m_container = container ? object : nullptr;
    if (container && m_sorter)
        container->appendSorter(m_sorter);

    Q_EMIT containerChanged();
}

SorterContainerAttached* SorterContainerAttached::qmlAttachedProperties(QObject* object)
{
    return new SorterContainerAttached(object);
}

}
