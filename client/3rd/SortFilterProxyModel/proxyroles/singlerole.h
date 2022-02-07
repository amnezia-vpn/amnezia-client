#ifndef SINGLEROLE_H
#define SINGLEROLE_H

#include "proxyrole.h"

namespace qqsfpm {

class SingleRole : public ProxyRole
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)

public:
    using ProxyRole::ProxyRole;

    QString name() const;
    void setName(const QString& name);

    QStringList names() override;

Q_SIGNALS:
    void nameChanged();

private:
    QString m_name;

private:
    QVariant data(const QModelIndex &sourceIndex, const QQmlSortFilterProxyModel &proxyModel, const QString &name) final;
    virtual QVariant data(const QModelIndex &sourceIndex, const QQmlSortFilterProxyModel &proxyModel) = 0;
};

}

#endif // SINGLEROLE_H
