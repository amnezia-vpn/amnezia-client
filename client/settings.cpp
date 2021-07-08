#include "defines.h"
#include "settings.h"
#include "utils.h"

#include <QDebug>
#include "protocols/protocols_defs.h"

const char Settings::cloudFlareNs1[] = "1.1.1.1";
const char Settings::cloudFlareNs2[] = "1.0.0.1";

Settings::Settings(QObject* parent) :
    QObject(parent),
    m_settings (ORGANIZATION_NAME, APPLICATION_NAME, this)
{
    // Import old settings
    if (serversCount() == 0) {
        QString user = m_settings.value("Server/userName").toString();
        QString password = m_settings.value("Server/password").toString();
        QString serverName = m_settings.value("Server/serverName").toString();
        int port = m_settings.value("Server/serverPort").toInt();

        if (!user.isEmpty() && !password.isEmpty() && !serverName.isEmpty()){
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
}

int Settings::serversCount() const
{
    return serversArray().size();
}

QJsonObject Settings::server(int index) const
{
    const QJsonArray &servers = serversArray();
    if (index >= servers.size()) return QJsonObject();

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
    if (index >= servers.size()) return;

    servers.removeAt(index);
    setServersArray(servers);
}

bool Settings::editServer(int index, const QJsonObject &server)
{
    QJsonArray servers = serversArray();
    if (index >= servers.size()) return false;

    servers.replace(index, server);
    setServersArray(servers);
    return true;
}

void Settings::setDefaultContainer(int serverIndex, DockerContainer container)
{
    QJsonObject s = server(serverIndex);
    s.insert(config_key::defaultContainer, containerToString(container));
    editServer(serverIndex, s);
}

DockerContainer Settings::defaultContainer(int serverIndex) const
{
    return containerFromString(defaultContainerName(serverIndex));
}

QString Settings::defaultContainerName(int serverIndex) const
{
    QString name = server(serverIndex).value(config_key::defaultContainer).toString();
    if (name.isEmpty()) {
        return containerToString(DockerContainer::None);
    }
    else return name;
}

QMap<DockerContainer, QJsonObject> Settings::containers(int serverIndex) const
{
    const QJsonArray &containers = server(serverIndex).value(config_key::containers).toArray();

    QMap<DockerContainer, QJsonObject> containersMap;
    for (const QJsonValue &val : containers) {
        containersMap.insert(containerFromString(val.toObject().value(config_key::container).toString()), val.toObject());
    }

    return containersMap;
}

void Settings::setContainers(int serverIndex, const QMap<DockerContainer, QJsonObject> &containers)
{
    QJsonObject s = server(serverIndex);
    QJsonArray c;
    for (const QJsonObject &o: containers) {
        c.append(o);
    }
    s.insert(config_key::containers, c);
    editServer(serverIndex, s);
}


QJsonObject Settings::containerConfig(int serverIndex, DockerContainer container)
{
    if (container == DockerContainer::None) return QJsonObject();
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
    c[container][config_key::container] = containerToString(container);
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

QJsonObject Settings::protocolConfig(int serverIndex, DockerContainer container, Protocol proto)
{
    const QJsonObject &c = containerConfig(serverIndex, container);
    return c.value(protoToString(proto)).toObject();
}

void Settings::setProtocolConfig(int serverIndex, DockerContainer container, Protocol proto, const QJsonObject &config)
{
    QJsonObject c = containerConfig(serverIndex, container);
    c.insert(protoToString(proto), config);

    setContainerConfig(serverIndex, container, c);
}

void Settings::clearLastConnectionConfig(int serverIndex, DockerContainer container, Protocol proto)
{
    if (proto == Protocol::Any) {
        for (Protocol p: { Protocol::OpenVpn, Protocol::ShadowSocks, Protocol::Cloak, Protocol::WireGuard}) {
            clearLastConnectionConfig(serverIndex, container, p);
        }
        return;
    }

    QJsonObject c = protocolConfig(serverIndex, container, proto);
    c.remove(config_key::last_config);
    setProtocolConfig(serverIndex, container, proto, c);
    qDebug() << "Settings::clearLastConnectionConfig for" << serverIndex << container << proto;
}

bool Settings::haveAuthData(int serverIndex) const
{
    if (serverIndex < 0) return false;
    ServerCredentials cred = serverCredentials(serverIndex);
    return (!cred.hostName.isEmpty() && !cred.userName.isEmpty() && !cred.password.isEmpty());
}

QString Settings::nextAvailableServerName() const
{
    int i = 0;
    bool nameExist = false;

    do {
        i++;
        nameExist = false;
        for (const QJsonValue &server: serversArray()) {
            if (server.toObject().value(config_key::description).toString() == tr("Server") + " " + QString::number(i)) {
                nameExist = true;
                break;
            }
        }
    } while (nameExist);

    return tr("Server") + " " + QString::number(i);
}

QString Settings::routeModeString(RouteMode mode) const
{
    switch (mode) {
    case VpnAllSites:
        return "AllSites";
    case VpnOnlyForwardSites:
        return "ForwardSites";
    case VpnAllExceptSites:
        return "ExceptSites";
    }
}

void Settings::addVpnSite(RouteMode mode, const QString &site, const QString &ip)
{
    QVariantMap sites = vpnSites(mode);
    if (sites.contains(site) && ip.isEmpty()) return;

    sites.insert(site, ip);
    setVpnSites(mode, sites);
}

QStringList Settings::getVpnIps(RouteMode mode) const
{
    QStringList ips;
    const QVariantMap &m = vpnSites(mode);
    for (auto i = m.constBegin(); i != m.constEnd(); ++i) {
        if (Utils::checkIpSubnetFormat(i.key())) {
            ips.append(i.key());
        }
        else if (Utils::checkIpSubnetFormat(i.value().toString())) {
            ips.append(i.value().toString());
        }
    }
    ips.removeDuplicates();
    return ips;
}

void Settings::removeVpnSite(RouteMode mode, const QString &site)
{
    QVariantMap sites = vpnSites(mode);
    if (!sites.contains(site)) return;

    sites.remove(site);
    setVpnSites(mode, sites);
}

void Settings::addVpnIps(RouteMode mode, const QStringList &ips)
{
    QVariantMap sites = vpnSites(mode);
    for (const QString &ip : ips) {
        if (ip.isEmpty()) continue;

        sites.insert(ip, "");
    }

    setVpnSites(mode, sites);
}

void Settings::removeVpnSites(RouteMode mode, const QStringList &sites)
{
    QVariantMap sitesMap = vpnSites(mode);
    for (const QString &site : sites) {
        if (site.isEmpty()) continue;

        sitesMap.remove(site);
    }

    setVpnSites(mode, sitesMap);
}

//void Settings::addVpnForwardSite(const QString &site, const QString &ip)
//{
//    auto sites = vpnForwardSites();
//    QStringList l = sites.value(site).toStringList();
//    if (!l.contains(ip)) {
//        l.append(ip);
//        setVpnForwardSites(sites);
//    }
//}

//QStringList Settings::getVpnForwardIps() const
//{
//    QStringList ips;
//    const QVariantMap &m = vpnForwardSites();
//    for (const QVariant &v : m) {
//        ips.append(v.toStringList());
//    }
//    ips.removeDuplicates();
//    return ips;
//}

//void Settings::addVpnExceptSite(const QString &site, const QString &ip)
//{
//    auto sites = vpnExceptSites();
//    QStringList l = sites.value(site).toStringList();
//    if (!l.contains(ip)) {
//        l.append(ip);
//        setVpnExceptSites(sites);
//    }
//}

//QStringList Settings::getVpnExceptIps() const
//{
//    QStringList ips;
//    const QVariantMap &m = vpnExceptSites();
//    for (const QVariant &v : m) {
//        ips.append(v.toStringList());
//    }
//    ips.removeDuplicates();
//    return ips;
//}

QString Settings::primaryDns() const { return m_settings.value("Conf/primaryDns", cloudFlareNs1).toString(); }

QString Settings::secondaryDns() const { return m_settings.value("Conf/secondaryDns", cloudFlareNs2).toString(); }

//void Settings::setServerCredentials(const ServerCredentials &credentials)
//{
//    setServerName(credentials.hostName);
//    setServerPort(credentials.port);
//    setUserName(credentials.userName);
//    setPassword(credentials.password);
//}

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
    credentials.password = s.value(config_key::password).toString();
    credentials.port = s.value(config_key::port).toInt();

    return credentials;
}
