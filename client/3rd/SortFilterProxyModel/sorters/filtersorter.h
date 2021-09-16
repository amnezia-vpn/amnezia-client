#ifndef FILTERSORTER_H
#define FILTERSORTER_H

#include "sorter.h"
#include "filters/filtercontainer.h"

namespace qqsfpm {

class FilterSorter : public Sorter, public FilterContainer
{
    Q_OBJECT
    Q_INTERFACES(qqsfpm::FilterContainer)
    Q_PROPERTY(QQmlListProperty<qqsfpm::Filter> filters READ filtersListProperty)
    Q_CLASSINFO("DefaultProperty", "filters")

public:
    using Sorter::Sorter;

protected:
    int compare(const QModelIndex &sourceLeft, const QModelIndex &sourceRight, const QQmlSortFilterProxyModel &proxyModel) const override;

private:
    void proxyModelCompleted(const QQmlSortFilterProxyModel& proxyModel) override;
    void onFilterAppended(Filter *filter) override;
    void onFilterRemoved(Filter *filter) override;
    void onFiltersCleared() override;

    bool indexIsAccepted(const QModelIndex &sourceIndex, const QQmlSortFilterProxyModel &proxyModel) const;
};

}

#endif // FILTERSORTER_H
