#include "allowedDnsController.h"

AllowedDnsController::AllowedDnsController(SecureAppSettingsRepository* appSettingsRepository)
    : m_appSettingsRepository(appSettingsRepository)
{
    fillDnsServers();
}

bool AllowedDnsController::addDns(const QString &ip)
{
    if (m_dnsServers.contains(ip)) {
        return false;
    }

    m_dnsServers.append(ip);
    m_appSettingsRepository->setAllowedDnsServers(m_dnsServers);
    return true;
}

void AllowedDnsController::addDnsList(const QStringList &dnsServers, bool replaceExisting)
{
    if (replaceExisting) {
        m_dnsServers.clear();
    }
    
    for (const QString &ip : dnsServers) {
        if (!m_dnsServers.contains(ip)) {
            m_dnsServers.append(ip);
        }
    }
    
    m_appSettingsRepository->setAllowedDnsServers(m_dnsServers);
}

void AllowedDnsController::removeDns(int index)
{
    if (index < 0 || index >= m_dnsServers.size()) {
        return;
    }

    m_dnsServers.removeAt(index);
    m_appSettingsRepository->setAllowedDnsServers(m_dnsServers);
}

QStringList AllowedDnsController::getCurrentDnsServers() const
{
    return m_dnsServers;
}

void AllowedDnsController::fillDnsServers()
{
    m_dnsServers = m_appSettingsRepository->getAllowedDnsServers();
}

