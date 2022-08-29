#ifndef INDEXSORTER_H
#define INDEXSORTER_H

#include "sorters/sorter.h"

class IndexSorter : public qqsfpm::Sorter
{
public:
    using qqsfpm::Sorter::Sorter;
    int compare(const QModelIndex& sourceLeft, const QModelIndex& sourceRight, const qqsfpm::QQmlSortFilterProxyModel& proxyModel) const override;
};

class ReverseIndexSorter : public qqsfpm::Sorter
{
public:
    using qqsfpm::Sorter::Sorter;
    int compare(const QModelIndex& sourceLeft, const QModelIndex& sourceRight, const qqsfpm::QQmlSortFilterProxyModel& proxyModel) const override;
};

#endif // INDEXSORTER_H
