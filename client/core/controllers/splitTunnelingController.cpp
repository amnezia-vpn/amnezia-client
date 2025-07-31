#include "splitTunnelingController.h"
#include "settings.h"
#include "core/networkUtilities.h"
#include <QFileInfo>

SplitTunnelingController::SplitTunnelingController(std::shared_ptr<Settings> settings, 
                                                   QSharedPointer<VpnConnection> vpnConnection,
                                                   QObject *parent)
    : QObject(parent), m_settings(settings), m_vpnConnection(vpnConnection)
{
}

// Apps split tunneling implementation
bool SplitTunnelingController::addApp(const InstalledAppInfo &appInfo)
{
    InstalledAppInfo processedAppInfo = appInfo;
    
    // Migrated from AppSplitTunnelingController::addApp - app name extraction
    if (processedAppInfo.appName.isEmpty() && !processedAppInfo.appPath.isEmpty()) {
        QFileInfo fileInfo(processedAppInfo.appPath);
        processedAppInfo.appName = fileInfo.fileName();
    }
    
    auto currentApps = m_settings->getVpnApps(getAppsRouteMode());
    
    if (currentApps.contains(processedAppInfo)) {
        return false;
    }
    
    currentApps.append(processedAppInfo);
    m_settings->setVpnApps(getAppsRouteMode(), currentApps);
    
    emit appAdded(processedAppInfo);
    return true;
}

bool SplitTunnelingController::removeApp(const InstalledAppInfo &appInfo)
{
    auto currentApps = m_settings->getVpnApps(getAppsRouteMode());
    
    if (!currentApps.contains(appInfo)) {
        return false;
    }
    
    currentApps.removeAll(appInfo);
    m_settings->setVpnApps(getAppsRouteMode(), currentApps);
    
    emit appRemoved(appInfo);
    return true;
}

QVector<InstalledAppInfo> SplitTunnelingController::getApps(Settings::AppsRouteMode routeMode) const
{
    return m_settings->getVpnApps(routeMode);
}

Settings::AppsRouteMode SplitTunnelingController::getAppsRouteMode() const
{
    return m_settings->getAppsRouteMode();
}

void SplitTunnelingController::setAppsRouteMode(Settings::AppsRouteMode routeMode)
{
    m_settings->setAppsRouteMode(routeMode);
    emit appsRouteModelChanged();
}

bool SplitTunnelingController::isAppsSplitTunnelingEnabled() const
{
    return m_settings->isAppsSplitTunnelingEnabled();
}

void SplitTunnelingController::setAppsSplitTunnelingEnabled(bool enabled)
{
    m_settings->setAppsSplitTunnelingEnabled(enabled);
    emit appsSplitTunnelingToggled();
}

// Sites split tunneling implementation
bool SplitTunnelingController::addSite(const QString &hostname, const QString &ip)
{
    QString processedHostname = hostname;
    
    // Migrated from SitesController::addSite - hostname processing
    if (!NetworkUtilities::ipAddressWithSubnetRegExp().exactMatch(hostname)) {
        processedHostname.replace("https://", "");
        processedHostname.replace("http://", "");
        processedHostname.replace("ftp://", "");
        processedHostname = processedHostname.split("/", Qt::SkipEmptyParts).first();
    }
    
    if (!m_settings->addVpnSite(getSitesRouteMode(), processedHostname, ip)) {
        return false;
    }
    
    // Migrated from SitesController::addSite - VPN route management
    if (m_vpnConnection) {
        if (!ip.isEmpty()) {
            QMetaObject::invokeMethod(m_vpnConnection.get(), "addRoutes", Qt::QueuedConnection,
                                      Q_ARG(QStringList, QStringList() << ip));
        } else if (NetworkUtilities::ipAddressWithSubnetRegExp().exactMatch(processedHostname)) {
            QMetaObject::invokeMethod(m_vpnConnection.get(), "addRoutes", Qt::QueuedConnection,
                                      Q_ARG(QStringList, QStringList() << processedHostname));
        } else {
            // Migrated from SitesController::addSite - DNS resolution
            int lookupId = QHostInfo::lookupHost(processedHostname, this, 
                                                SLOT(handleHostnameResolved(QHostInfo)));
            m_pendingResolutions[lookupId] = processedHostname;
        }
    }
    
    emit siteAdded(processedHostname, ip);
    return true;
}

bool SplitTunnelingController::addSites(const QMap<QString, QString> &sites, bool replaceExisting)
{
    if (replaceExisting) {
        m_settings->removeAllVpnSites(getSitesRouteMode());
    }
    
    m_settings->addVpnSites(getSitesRouteMode(), sites);
    
    // Migrated from SitesController - VPN route management for batch adds
    if (m_vpnConnection) {
        QStringList ips;
        auto i = sites.constBegin();
        while (i != sites.constEnd()) {
            const QString &hostname = i.key();
            const QString &ip = i.value();
            
            if (ip.isEmpty()) {
                ips.append(hostname);
            } else {
                ips.append(ip);
            }
            ++i;
        }
        
        QMetaObject::invokeMethod(m_vpnConnection.get(), "addRoutes", Qt::QueuedConnection, 
                                  Q_ARG(QStringList, ips));
    }
    
    auto i = sites.constBegin();
    while (i != sites.constEnd()) {
        emit siteAdded(i.key(), i.value());
        ++i;
    }
    
    return true;
}

bool SplitTunnelingController::removeSite(const QString &hostname)
{
    if (!m_settings->removeVpnSite(getSitesRouteMode(), hostname)) {
        return false;
    }
    
    // Migrated from SitesController::removeSite - VPN route management
    if (m_vpnConnection) {
        QMetaObject::invokeMethod(m_vpnConnection.get(), "deleteRoutes", Qt::QueuedConnection,
                                  Q_ARG(QStringList, QStringList() << hostname));
    }
    
    emit siteRemoved(hostname);
    return true;
}

// Migrated from SitesController - DNS resolution handler
void SplitTunnelingController::handleHostnameResolved(const QHostInfo &hostInfo)
{
    if (hostInfo.error() != QHostInfo::NoError) {
        return;
    }
    
    QString hostname = m_pendingResolutions.take(hostInfo.lookupId());
    if (hostname.isEmpty()) {
        return;
    }
    
    for (const QHostAddress &addr : hostInfo.addresses()) {
        if (addr.protocol() == QAbstractSocket::NetworkLayerProtocol::IPv4Protocol) {
            if (m_vpnConnection) {
                QMetaObject::invokeMethod(m_vpnConnection.get(), "addRoutes", Qt::QueuedConnection,
                                          Q_ARG(QStringList, QStringList() << addr.toString()));
            }
            
            m_settings->addVpnSite(getSitesRouteMode(), hostname, addr.toString());
            break;
        }
    }
}

QVector<QPair<QString, QString>> SplitTunnelingController::getSites(Settings::RouteMode routeMode) const
{
    QVector<QPair<QString, QString>> sites;
    const QVariantMap &sitesMap = m_settings->vpnSites(routeMode);
    
    auto i = sitesMap.constBegin();
    while (i != sitesMap.constEnd()) {
        sites.append(qMakePair(i.key(), i.value().toString()));
        ++i;
    }
    
    return sites;
}

Settings::RouteMode SplitTunnelingController::getSitesRouteMode() const
{
    return m_settings->routeMode();
}

void SplitTunnelingController::setSitesRouteMode(Settings::RouteMode routeMode)
{
    m_settings->setRouteMode(routeMode);
    emit sitesRouteModelChanged();
}

bool SplitTunnelingController::isSitesSplitTunnelingEnabled() const
{
    return m_settings->isSitesSplitTunnelingEnabled();
}

void SplitTunnelingController::setSitesSplitTunnelingEnabled(bool enabled)
{
    m_settings->setSitesSplitTunnelingEnabled(enabled);
    emit sitesSplitTunnelingToggled();
} 
