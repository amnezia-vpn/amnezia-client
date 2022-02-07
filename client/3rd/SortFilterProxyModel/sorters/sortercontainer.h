#ifndef SORTERSSORTERCONTAINER_H
#define SORTERSSORTERCONTAINER_H

#include <QList>
#include <QQmlListProperty>
#include <qqml.h>
#include <QPointer>

namespace qqsfpm {

class Sorter;
class QQmlSortFilterProxyModel;

class SorterContainer {
public:
    virtual ~SorterContainer() = default;

    QList<Sorter*> sorters() const;
    void appendSorter(Sorter* sorter);
    void removeSorter(Sorter* sorter);
    void clearSorters();

    QQmlListProperty<Sorter> sortersListProperty();

protected:
    QList<Sorter*> m_sorters;

private:
    virtual void onSorterAppended(Sorter* sorter) = 0;
    virtual void onSorterRemoved(Sorter* sorter) = 0;
    virtual void onSortersCleared() = 0;

    static void append_sorter(QQmlListProperty<Sorter>* list, Sorter* sorter);
    static int count_sorter(QQmlListProperty<Sorter>* list);
    static Sorter* at_sorter(QQmlListProperty<Sorter>* list, int index);
    static void clear_sorters(QQmlListProperty<Sorter>* list);
};

class SorterContainerAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QObject* container READ container WRITE setContainer NOTIFY containerChanged)

public:
    SorterContainerAttached(QObject* object);
    ~SorterContainerAttached();

    QObject* container() const;
    void setContainer(QObject* object);

    static SorterContainerAttached* qmlAttachedProperties(QObject* object);

Q_SIGNALS:
    void containerChanged();

private:
    QPointer<QObject> m_container = nullptr;
    Sorter* m_sorter = nullptr;
};

}

#define SorterContainer_iid "fr.grecko.SortFilterProxyModel.SorterContainer"
Q_DECLARE_INTERFACE(qqsfpm::SorterContainer, SorterContainer_iid)

QML_DECLARE_TYPEINFO(qqsfpm::SorterContainerAttached, QML_HAS_ATTACHED_PROPERTIES)

#endif // SORTERSSORTERCONTAINER_H
