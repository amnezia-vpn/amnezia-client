#ifndef RANGEFILTER_H
#define RANGEFILTER_H

#include "rolefilter.h"
#include <QVariant>

namespace qqsfpm {

class RangeFilter : public RoleFilter
{
    Q_OBJECT
    Q_PROPERTY(QVariant minimumValue READ minimumValue WRITE setMinimumValue NOTIFY minimumValueChanged)
    Q_PROPERTY(bool minimumInclusive READ minimumInclusive WRITE setMinimumInclusive NOTIFY minimumInclusiveChanged)
    Q_PROPERTY(QVariant maximumValue READ maximumValue WRITE setMaximumValue NOTIFY maximumValueChanged)
    Q_PROPERTY(bool maximumInclusive READ maximumInclusive WRITE setMaximumInclusive NOTIFY maximumInclusiveChanged)

public:
    using RoleFilter::RoleFilter;

    QVariant minimumValue() const;
    void setMinimumValue(QVariant minimumValue);
    bool minimumInclusive() const;
    void setMinimumInclusive(bool minimumInclusive);

    QVariant maximumValue() const;
    void setMaximumValue(QVariant maximumValue);
    bool maximumInclusive() const;
    void setMaximumInclusive(bool maximumInclusive);

protected:
    bool filterRow(const QModelIndex& sourceIndex, const QQmlSortFilterProxyModel& proxyModel) const override;

Q_SIGNALS:
    void minimumValueChanged();
    void minimumInclusiveChanged();
    void maximumValueChanged();
    void maximumInclusiveChanged();

private:
    QVariant m_minimumValue;
    bool m_minimumInclusive = true;
    QVariant m_maximumValue;
    bool m_maximumInclusive = true;
};

}

#endif // RANGEFILTER_H
