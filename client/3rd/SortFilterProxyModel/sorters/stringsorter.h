#ifndef STRINGSORTER_H
#define STRINGSORTER_H

#include "rolesorter.h"
#include <QCollator>

namespace qqsfpm {

class StringSorter : public RoleSorter
{
    Q_OBJECT
    Q_PROPERTY(Qt::CaseSensitivity caseSensitivity READ caseSensitivity WRITE setCaseSensitivity NOTIFY caseSensitivityChanged)
    Q_PROPERTY(bool ignorePunctation READ ignorePunctation WRITE setIgnorePunctation NOTIFY ignorePunctationChanged)
    Q_PROPERTY(QLocale locale READ locale WRITE setLocale NOTIFY localeChanged)
    Q_PROPERTY(bool numericMode READ numericMode WRITE setNumericMode NOTIFY numericModeChanged)

public:
    using RoleSorter::RoleSorter;

    Qt::CaseSensitivity caseSensitivity() const;
    void setCaseSensitivity(Qt::CaseSensitivity caseSensitivity);

    bool ignorePunctation() const;
    void setIgnorePunctation(bool ignorePunctation);

    QLocale locale() const;
    void setLocale(const QLocale& locale);

    bool numericMode() const;
    void setNumericMode(bool numericMode);

Q_SIGNALS:
    void caseSensitivityChanged();
    void ignorePunctationChanged();
    void localeChanged();
    void numericModeChanged();

protected:
    int compare(const QModelIndex& sourceLeft, const QModelIndex& sourceRight, const QQmlSortFilterProxyModel& proxyModel) const override;

private:
    QCollator m_collator;
};

}

#endif // STRINGSORTER_H
