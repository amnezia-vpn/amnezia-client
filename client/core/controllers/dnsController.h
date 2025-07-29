#ifndef DNSCONTROLLER_H
#define DNSCONTROLLER_H

#include <QObject>
#include <QStringList>
#include <QSharedPointer>

#include "settings.h"

class DnsController : public QObject
{
    Q_OBJECT

public:
    explicit DnsController(std::shared_ptr<Settings> settings, QObject *parent = nullptr);

    // DNS management
    bool addDns(const QString &ip);
    bool addDnsList(const QStringList &dnsServers, bool replaceExisting = false);
    bool removeDns(const QString &ip);
    QStringList getAllowedDnsServers() const;

signals:
    void dnsAdded(const QString &ip);
    void dnsListAdded(const QStringList &dnsServers);
    void dnsRemoved(const QString &ip);

private:
    std::shared_ptr<Settings> m_settings;
};

#endif // DNSCONTROLLER_H 
