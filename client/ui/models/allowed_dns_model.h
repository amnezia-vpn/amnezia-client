#ifndef ALLOWEDDNSMODEL_H
#define ALLOWEDDNSMODEL_H

#include <QAbstractListModel>
#include "settings.h"

class AllowedDnsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        IpRole = Qt::UserRole + 1
    };

    explicit AllowedDnsModel(std::shared_ptr<Settings> settings, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

public slots:
    bool addDns(const QString &ip);
    void addDnsList(const QStringList &dnsServers, bool replaceExisting);
    void removeDns(QModelIndex index);
    QStringList getCurrentDnsServers();

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    void fillDnsServers();

    std::shared_ptr<Settings> m_settings;
    QStringList m_dnsServers;
};

#endif // ALLOWEDDNSMODEL_H
