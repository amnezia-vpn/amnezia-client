#include "settings.h"

#include "QCoreApplication"
#include "QThread"

#include "core/networkUtilities.h"
#include "version.h"

#include "containers/containers_defs.h"
#include "logger.h"

namespace
{
    const char cloudFlareNs1[] = "1.1.1.1";
    const char cloudFlareNs2[] = "1.0.0.1";

    constexpr char gatewayEndpoint[] = "http://gw.amnezia.org:80/";
}

Settings::Settings(QObject *parent) : QObject(parent), m_settings(ORGANIZATION_NAME, APPLICATION_NAME, this)
{
    // Import old settings
    if (serversCount() == 0) {
        QString user = value("Server/userName").toString();
        QString password = value("Server/password").toString();
        QString serverName = value("Server/serverName").toString();
        int port = value("Server/serverPort").toInt();

        if (!user.isEmpty() && !password.isEmpty() && !serverName.isEmpty()) {
            QJsonObject server;
            server.insert(config_key::userName, user);
            server.insert(config_key::password, password);
            server.insert(config_key::hostName, serverName);
            server.insert(config_key::port, port);
            server.insert(config_key::description, tr("Server #1"));

            addServer(server);

            m_settings.remove("Server/userName");
            m_settings.remove("Server/password");
            m_settings.remove("Server/serverName");
            m_settings.remove("Server/serverPort");
        }
    }

    m_gatewayEndpoint = gatewayEndpoint;
}

int Settings::serversCount() const
{
    return serversArray().size();
}

QJsonObject Settings::server(int index) const
{
    const QJsonArray &servers = serversArray();
    if (index >= servers.size())
        return QJsonObject();

    return servers.at(index).toObject();
}

void Settings::addServer(const QJsonObject &server)
{
    QJsonArray servers = serversArray();
    servers.append(server);
    setServersArray(servers);
}

void Settings::removeServer(int index)
{
    QJsonArray servers = serversArray();
    if (index >= servers.size())
        return;

    servers.removeAt(index);
    setServersArray(servers);
    emit serverRemoved(index);
}

bool Settings::editServer(int index, const QJsonObject &server)
{
    QJsonArray servers = serversArray();
    if (index >= servers.size())
        return false;

    servers.replace(index, server);
    setServersArray(servers);
    return true;
}

void Settings::setDefaultContainer(int serverIndex, DockerContainer container)
{
    QJsonObject s = server(serverIndex);
    s.insert(config_key::defaultContainer, ContainerProps::containerToString(container));
    editServer(serverIndex, s);
}

DockerContainer Settings::defaultContainer(int serverIndex) const
{
    return ContainerProps::containerFromString(defaultContainerName(serverIndex));
}

QString Settings::defaultContainerName(int serverIndex) const
{
    QString name = server(serverIndex).value(config_key::defaultContainer).toString();
    if (name.isEmpty()) {
        return ContainerProps::containerToString(DockerContainer::None);
    } else
        return name;
}

QMap<DockerContainer, QJsonObject> Settings::containers(int serverIndex) const
{
    const QJsonArray &containers = server(serverIndex).value(config_key::containers).toArray();

    QMap<DockerContainer, QJsonObject> containersMap;
    for (const QJsonValue &val : containers) {
        containersMap.insert(ContainerProps::containerFromString(val.toObject().value(config_key::container).toString()), val.toObject());
    }

    return containersMap;
}

void Settings::setContainers(int serverIndex, const QMap<DockerContainer, QJsonObject> &containers)
{
    QJsonObject s = server(serverIndex);
    QJsonArray c;
    for (const QJsonObject &o : containers) {
        c.append(o);
    }
    s.insert(config_key::containers, c);
    editServer(serverIndex, s);
}

QJsonObject Settings::containerConfig(int serverIndex, DockerContainer container)
{
    if (container == DockerContainer::None)
        return QJsonObject();
    return containers(serverIndex).value(container);
}

void Settings::setContainerConfig(int serverIndex, DockerContainer container, const QJsonObject &config)
{
    if (container == DockerContainer::None) {
        qCritical() << "Settings::setContainerConfig trying to set config for container == DockerContainer::None";
        return;
    }
    auto c = containers(serverIndex);
    c[container] = config;
    c[container][config_key::container] = ContainerProps::containerToString(container);
    setContainers(serverIndex, c);
}

void Settings::removeContainerConfig(int serverIndex, DockerContainer container)
{
    if (container == DockerContainer::None) {
        qCritical() << "Settings::removeContainerConfig trying to remove config for container == DockerContainer::None";
        return;
    }

    auto c = containers(serverIndex);
    c.remove(container);
    setContainers(serverIndex, c);
}

QJsonObject Settings::protocolConfig(int serverIndex, DockerContainer container, Proto proto)
{
    const QJsonObject &c = containerConfig(serverIndex, container);
    return c.value(ProtocolProps::protoToString(proto)).toObject();
}

void Settings::setProtocolConfig(int serverIndex, DockerContainer container, Proto proto, const QJsonObject &config)
{
    QJsonObject c = containerConfig(serverIndex, container);
    c.insert(ProtocolProps::protoToString(proto), config);

    setContainerConfig(serverIndex, container, c);
}

void Settings::clearLastConnectionConfig(int serverIndex, DockerContainer container, Proto proto)
{
    // recursively remove
    if (proto == Proto::Any) {
        for (Proto p : ContainerProps::protocolsForContainer(container)) {
            clearLastConnectionConfig(serverIndex, container, p);
        }
        return;
    }

    QJsonObject c = protocolConfig(serverIndex, container, proto);
    c.remove(config_key::last_config);
    setProtocolConfig(serverIndex, container, proto, c);
}

bool Settings::haveAuthData(int serverIndex) const
{
    if (serverIndex < 0)
        return false;
    ServerCredentials cred = serverCredentials(serverIndex);
    return (!cred.hostName.isEmpty() && !cred.userName.isEmpty() && !cred.secretData.isEmpty());
}

QString Settings::nextAvailableServerName() const
{
    int i = 0;
    bool nameExist = false;

    do {
        i++;
        nameExist = false;
        for (const QJsonValue &server : serversArray()) {
            if (server.toObject().value(config_key::description).toString() == tr("Server") + " " + QString::number(i)) {
                nameExist = true;
                break;
            }
        }
    } while (nameExist);

    return tr("Server") + " " + QString::number(i);
}

void Settings::setSaveLogs(bool enabled)
{
    setValue("Conf/saveLogs", enabled);
#ifndef Q_OS_ANDROID
    if (!isSaveLogs()) {
        Logger::deInit();
    } else {
        if (!Logger::init(false)) {
            qWarning() << "Initialization of debug subsystem failed";
        }
    }
#endif
    Logger::setServiceLogsEnabled(enabled);

    if (enabled) {
        setLogEnableDate(QDateTime::currentDateTime());
    }
    emit saveLogsChanged(enabled);
}

QDateTime Settings::getLogEnableDate()
{
    return value("Conf/logEnableDate").toDateTime();
}

void Settings::setLogEnableDate(QDateTime date)
{
    setValue("Conf/logEnableDate", date);
}

QString Settings::routeModeString(RouteMode mode) const
{
    switch (mode) {
    case VpnAllSites: return "AllSites";
    case VpnOnlyForwardSites: return "ForwardSites";
    case VpnAllExceptSites: return "ExceptSites";
    }
}

Settings::RouteMode Settings::routeMode() const
{
    return static_cast<RouteMode>(value("Conf/routeMode", 0).toInt());
}

bool Settings::isSitesSplitTunnelingEnabled() const
{
    return value("Conf/sitesSplitTunnelingEnabled", false).toBool();
}

void Settings::setSitesSplitTunnelingEnabled(bool enabled)
{
    setValue("Conf/sitesSplitTunnelingEnabled", enabled);
}

bool Settings::addVpnSite(RouteMode mode, const QString &site, const QString &ip)
{
    QVariantMap sites = vpnSites(mode);
    if (sites.contains(site) && ip.isEmpty())
        return false;

    sites.insert(site, ip);
    setVpnSites(mode, sites);
    return true;
}

void Settings::addVpnSites(RouteMode mode, const QMap<QString, QString> &sites)
{
    QVariantMap allSites = vpnSites(mode);
    for (auto i = sites.constBegin(); i != sites.constEnd(); ++i) {
        const QString &site = i.key();
        const QString &ip = i.value();

        if (allSites.contains(site) && allSites.value(site) == ip)
            continue;

        allSites.insert(site, ip);
    }

    setVpnSites(mode, allSites);
}

QStringList Settings::getVpnIps(RouteMode mode) const
{
    QStringList ips;
    const QVariantMap &m = vpnSites(mode);
    for (auto i = m.constBegin(); i != m.constEnd(); ++i) {
        if (NetworkUtilities::checkIpSubnetFormat(i.key())) {
            ips.append(i.key());
        } else if (NetworkUtilities::checkIpSubnetFormat(i.value().toString())) {
            ips.append(i.value().toString());
        }
    }
    ips.removeDuplicates();
    return ips;
}

void Settings::removeVpnSite(RouteMode mode, const QString &site)
{
    QVariantMap sites = vpnSites(mode);
    if (!sites.contains(site))
        return;

    sites.remove(site);
    setVpnSites(mode, sites);
}

void Settings::addVpnIps(RouteMode mode, const QStringList &ips)
{
    QVariantMap sites = vpnSites(mode);
    for (const QString &ip : ips) {
        if (ip.isEmpty())
            continue;

        sites.insert(ip, "");
    }

    setVpnSites(mode, sites);
}

void Settings::removeVpnSites(RouteMode mode, const QStringList &sites)
{
    QVariantMap sitesMap = vpnSites(mode);
    for (const QString &site : sites) {
        if (site.isEmpty())
            continue;

        sitesMap.remove(site);
    }

    setVpnSites(mode, sitesMap);
}

void Settings::removeAllVpnSites(RouteMode mode)
{
    setVpnSites(mode, QVariantMap());
}

QString Settings::primaryDns() const
{
    return value("Conf/primaryDns", cloudFlareNs1).toString();
}

QString Settings::secondaryDns() const
{
    return value("Conf/secondaryDns", cloudFlareNs2).toString();
}

void Settings::clearSettings()
{
    auto uuid = getInstallationUuid(false);
    m_settings.clearSettings();
    setInstallationUuid(uuid);
    emit settingsCleared();
}

QString Settings::appsRouteModeString(AppsRouteMode mode) const
{
    switch (mode) {
    case VpnAllApps: return "AllApps";
    case VpnOnlyForwardApps: return "ForwardApps";
    case VpnAllExceptApps: return "ExceptApps";
    }
}

Settings::AppsRouteMode Settings::getAppsRouteMode() const
{
    return static_cast<AppsRouteMode>(value("Conf/appsRouteMode", 0).toInt());
}

void Settings::setAppsRouteMode(AppsRouteMode mode)
{
    setValue("Conf/appsRouteMode", mode);
}

QVector<InstalledAppInfo> Settings::getVpnApps(AppsRouteMode mode) const
{
    QVector<InstalledAppInfo> apps;
    auto appsArray = value("Conf/" + appsRouteModeString(mode)).toJsonArray();
    for (const auto &app : appsArray) {
        InstalledAppInfo appInfo;
        appInfo.appName = app.toObject().value("appName").toString();
        appInfo.packageName = app.toObject().value("packageName").toString();
        appInfo.appPath = app.toObject().value("appPath").toString();

        apps.push_back(appInfo);
    }
    return apps;
}

void Settings::setVpnApps(AppsRouteMode mode, const QVector<InstalledAppInfo> &apps)
{
    QJsonArray appsArray;
    for (const auto &app : apps) {
        QJsonObject appInfo;
        appInfo.insert("appName", app.appName);
        appInfo.insert("packageName", app.packageName);
        appInfo.insert("appPath", app.appPath);
        appsArray.push_back(appInfo);
    }
    setValue("Conf/" + appsRouteModeString(mode), appsArray);
    m_settings.sync();
}

bool Settings::isAppsSplitTunnelingEnabled() const
{
    return value("Conf/appsSplitTunnelingEnabled", false).toBool();
}

void Settings::setAppsSplitTunnelingEnabled(bool enabled)
{
    setValue("Conf/appsSplitTunnelingEnabled", enabled);
}

bool Settings::isKillSwitchEnabled() const
{
    return value("Conf/killSwitchEnabled", true).toBool();
}

void Settings::setKillSwitchEnabled(bool enabled)
{
    setValue("Conf/killSwitchEnabled", enabled);
}

QString Settings::getInstallationUuid(const bool needCreate)
{
    auto uuid = value("Conf/installationUuid", "").toString();
    if (needCreate && uuid.isEmpty()) {
        uuid = QUuid::createUuid().toString();

        //remove {} from uuid
        uuid.remove(0, 1);
        uuid.chop(1);

        setInstallationUuid(uuid);
    } else if (uuid.contains("{") && uuid.contains("}")) {
        //remove {} from old uuid
        uuid.remove(0, 1);
        uuid.chop(1);

        setInstallationUuid(uuid);
    }
    return uuid;
}

void Settings::setInstallationUuid(const QString &uuid)
{
    setValue("Conf/installationUuid", uuid);
}

ServerCredentials Settings::defaultServerCredentials() const
{
    return serverCredentials(defaultServerIndex());
}

ServerCredentials Settings::serverCredentials(int index) const
{
    const QJsonObject &s = server(index);

    ServerCredentials credentials;
    credentials.hostName = s.value(config_key::hostName).toString();
    credentials.userName = s.value(config_key::userName).toString();
    credentials.secretData = s.value(config_key::password).toString();
    credentials.port = s.value(config_key::port).toInt();

    return credentials;
}

QVariant Settings::value(const QString &key, const QVariant &defaultValue) const
{
    QVariant returnValue;
    if (QThread::currentThread() == QCoreApplication::instance()->thread()) {
        returnValue = m_settings.value(key, defaultValue);
    } else {
        QMetaObject::invokeMethod(&m_settings, "value", Qt::BlockingQueuedConnection, Q_RETURN_ARG(QVariant, returnValue),
                                  Q_ARG(const QString &, key), Q_ARG(const QVariant &, defaultValue));
    }
    return returnValue;
}

void Settings::setValue(const QString &key, const QVariant &value)
{
    if (QThread::currentThread() == QCoreApplication::instance()->thread()) {
        m_settings.setValue(key, value);
    } else {
        QMetaObject::invokeMethod(&m_settings, "setValue", Qt::BlockingQueuedConnection, Q_ARG(const QString &, key),
                                  Q_ARG(const QVariant &, value));
    }
}

void Settings::resetGatewayEndpoint()
{
    m_gatewayEndpoint = gatewayEndpoint;
}

void Settings::setGatewayEndpoint(const QString &endpoint)
{
    m_gatewayEndpoint = endpoint;
}

void Settings::setDevGatewayEndpoint()
{
    m_gatewayEndpoint = DEV_AGW_ENDPOINT;
}

QString Settings::getGatewayEndpoint()
{
    return m_gatewayEndpoint;
}

bool Settings::isDevGatewayEnv()
{
    return m_isDevGatewayEnv;
}

void Settings::toggleDevGatewayEnv(bool enabled)
{
    m_isDevGatewayEnv = enabled;
}

bool Settings::isHomeAdLabelVisible()
{
    return value("Conf/homeAdLabelVisible", true).toBool();
}

void Settings::disableHomeAdLabel()
{
    setValue("Conf/homeAdLabelVisible", false);
}
