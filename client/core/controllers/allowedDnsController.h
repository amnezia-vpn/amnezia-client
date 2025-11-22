#ifndef ALLOWEDDNSCONTROLLER_H
#define ALLOWEDDNSCONTROLLER_H

#include <QStringList>

#include "settings.h"

class AllowedDnsController
{
public:
    explicit AllowedDnsController(std::shared_ptr<Settings> settings);

    bool addDns(const QString &ip);
    void addDnsList(const QStringList &dnsServers, bool replaceExisting);
    void removeDns(int index);
    QStringList getCurrentDnsServers() const;

private:
    void fillDnsServers();

    std::shared_ptr<Settings> m_settings;
    QStringList m_dnsServers;
};

#endif // ALLOWEDDNSCONTROLLER_H

