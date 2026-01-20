#ifndef ALLOWEDDNSCONTROLLER_H
#define ALLOWEDDNSCONTROLLER_H

#include <QStringList>

#include "core/repositories/qAppSettingsRepository.h"

class AllowedDnsController
{
public:
    explicit AllowedDnsController(QAppSettingsRepository* appSettingsRepository);

    bool addDns(const QString &ip);
    void addDnsList(const QStringList &dnsServers, bool replaceExisting);
    void removeDns(int index);
    QStringList getCurrentDnsServers() const;

private:
    void fillDnsServers();

    QAppSettingsRepository* m_appSettingsRepository;
    QStringList m_dnsServers;
};

#endif // ALLOWEDDNSCONTROLLER_H

