#ifndef FILTERCONTAINER_H
#define FILTERCONTAINER_H

#include <QList>
#include <QQmlListProperty>
#include <qqml.h>
#include <QPointer>

namespace qqsfpm {

class Filter;
class QQmlSortFilterProxyModel;

class FilterContainer {
public:
    virtual ~FilterContainer() = default;

    QList<Filter*> filters() const;
    void appendFilter(Filter* filter);
    void removeFilter(Filter* filter);
    void clearFilters();

    QQmlListProperty<Filter> filtersListProperty();

protected:
    QList<Filter*> m_filters;

private:
    virtual void onFilterAppended(Filter* filter) = 0;
    virtual void onFilterRemoved(Filter* filter) = 0;
    virtual void onFiltersCleared() = 0;

    static void append_filter(QQmlListProperty<Filter>* list, Filter* filter);
    static int count_filter(QQmlListProperty<Filter>* list);
    static Filter* at_filter(QQmlListProperty<Filter>* list, int index);
    static void clear_filters(QQmlListProperty<Filter>* list);
};

class FilterContainerAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QObject* container READ container WRITE setContainer NOTIFY containerChanged)

public:
    FilterContainerAttached(QObject* object);
    ~FilterContainerAttached();

    QObject* container() const;
    void setContainer(QObject* object);

    static FilterContainerAttached* qmlAttachedProperties(QObject* object);

Q_SIGNALS:
    void containerChanged();

private:
    QPointer<QObject> m_container = nullptr;
    Filter* m_filter = nullptr;
};

}

#define FilterContainer_iid "fr.grecko.SortFilterProxyModel.FilterContainer"
Q_DECLARE_INTERFACE(qqsfpm::FilterContainer, FilterContainer_iid)

QML_DECLARE_TYPEINFO(qqsfpm::FilterContainerAttached, QML_HAS_ATTACHED_PROPERTIES)

#endif // FILTERCONTAINER_H
