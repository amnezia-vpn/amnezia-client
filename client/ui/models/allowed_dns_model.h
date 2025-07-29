#ifndef ALLOWEDDNSMODEL_H
#define ALLOWEDDNSMODEL_H

#include <QAbstractListModel>
#include "settings.h"

class DnsController;

class AllowedDnsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        IpRole = Qt::UserRole + 1
    };

    explicit AllowedDnsModel(std::shared_ptr<Settings> settings, 
                            QSharedPointer<DnsController> dnsController,
                            QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

public slots:
    bool addDns(const QString &ip);
    void addDnsList(const QStringList &dnsServers, bool replaceExisting);
    void removeDns(QModelIndex index);
    QStringList getCurrentDnsServers();

protected:
    QHash<int, QByteArray> roleNames() const override;

private slots:
    void onDnsAdded(const QString &ip);
    void onDnsListAdded(const QStringList &dnsServers);
    void onDnsRemoved(const QString &ip);

private:
    void refreshData();

    std::shared_ptr<Settings> m_settings;
    QSharedPointer<DnsController> m_dnsController;
    QStringList m_dnsServers;
};

#endif // ALLOWEDDNSMODEL_H
