#include "dnsController.h"

DnsController::DnsController(std::shared_ptr<Settings> settings, QObject *parent)
    : QObject(parent), m_settings(settings)
{
}

bool DnsController::addDns(const QString &ip)
{
    QStringList currentDnsServers = m_settings->allowedDnsServers();
    
    if (currentDnsServers.contains(ip)) {
        return false;
    }
    
    currentDnsServers.append(ip);
    m_settings->setAllowedDnsServers(currentDnsServers);
    
    emit dnsAdded(ip);
    return true;
}

bool DnsController::addDnsList(const QStringList &dnsServers, bool replaceExisting)
{
    QStringList currentDnsServers;
    
    if (!replaceExisting) {
        currentDnsServers = m_settings->allowedDnsServers();
    }
    
    for (const QString &ip : dnsServers) {
        if (!currentDnsServers.contains(ip)) {
            currentDnsServers.append(ip);
        }
    }
    
    m_settings->setAllowedDnsServers(currentDnsServers);
    
    emit dnsListAdded(dnsServers);
    return true;
}

bool DnsController::removeDns(const QString &ip)
{
    QStringList currentDnsServers = m_settings->allowedDnsServers();
    
    if (!currentDnsServers.contains(ip)) {
        return false;
    }
    
    currentDnsServers.removeAll(ip);
    m_settings->setAllowedDnsServers(currentDnsServers);
    
    emit dnsRemoved(ip);
    return true;
}

QStringList DnsController::getAllowedDnsServers() const
{
    return m_settings->allowedDnsServers();
} 
