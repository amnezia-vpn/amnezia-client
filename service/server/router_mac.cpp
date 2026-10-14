#include "router_mac.h"
#include "helper_route_mac.h"

#include <QNetworkInterface>
#include <QProcess>
#include <QThread>

#include <core/utils/networkUtilities.h>

namespace {
void runRoute(const QString &cmd)
{
    const QStringList parts = cmd.split(' ');
    const int argc = parts.size();
    char **argv = new char*[argc];
    for (int i = 0; i < argc; ++i) {
        const QByteArray arg = parts.at(i).toUtf8();
        argv[i] = new char[arg.size() + 1];
        strcpy(argv[i], arg.constData());
    }
    mainRouteIface(argc, argv);
    for (int i = 0; i < argc; ++i) delete[] argv[i];
    delete[] argv;
}

QPair<QString, QString> onLinkInterface(const QString &destination, const QString &gateway)
{
    const auto subnet = QHostAddress::parseSubnet(destination);
    const QHostAddress router(gateway);
    if (subnet.second < 0 || subnet.second == 32 || router.protocol() != QAbstractSocket::IPv4Protocol)
        return {};

    for (const QNetworkInterface &iface : QNetworkInterface::allInterfaces()) {
        if (!(iface.flags() & QNetworkInterface::IsUp) || (iface.flags() & QNetworkInterface::IsLoopBack))
            continue;
        for (const QNetworkAddressEntry &entry : iface.addressEntries()) {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol && entry.prefixLength() >= 0 &&
                entry.prefixLength() <= subnet.second &&
                router.isInSubnet(entry.ip(), entry.prefixLength()) &&
                subnet.first.isInSubnet(entry.ip(), entry.prefixLength()))
                return {iface.name(), entry.ip().toString()};
        }
    }
    return {};
}
}

RouterMac &RouterMac::Instance()
{
    static RouterMac s;
    return s;
}

bool RouterMac::routeAdd(const QString &ipWithSubnet, const QString &gw)
{
    QString ip = NetworkUtilities::ipAddressFromIpWithSubnet(ipWithSubnet);
    QString mask = NetworkUtilities::netMaskFromIpWithSubnet(ipWithSubnet);

#ifdef MZ_DEBUG
    qDebug().noquote() << "RouterMac::routeAdd: " << ipWithSubnet << gw;
#endif

    if (!NetworkUtilities::checkIPv4Format(ip) || !NetworkUtilities::checkIPv4Format(gw)) {
        qCritical().noquote() << "Critical, trying to add invalid route: " << ip << gw;
        return false;
    }

    const auto [iface, localIp] = onLinkInterface(ipWithSubnet, gw);
    if (!iface.isEmpty()) {
        const QStringList args = {"-n", "add", "-net", ip, "-netmask", mask, "-interface", localIp};
        // XRay has interface-scoped /1 routes, so both route scopes need the on-link prefix.
        const bool directAdded = QProcess::execute("/sbin/route", args) == 0;
        const bool scopedAdded = QProcess::execute("/sbin/route", args + QStringList{"-ifscope", iface}) == 0;
        if (!directAdded && !scopedAdded) {
            qWarning().noquote() << "Failed to add on-link route:" << ipWithSubnet << iface;
            return false;
        }
        m_addedRoutes.append({ipWithSubnet, gw, iface, directAdded, scopedAdded});
        return true;
    } else if (mask == "255.255.255.255") {
        runRoute(QString("route add -host %1 %2").arg(ip, gw));
    } else {
        runRoute(QString("route add -net %1 %2 %3").arg(ip, gw, mask));
    }
    m_addedRoutes.append({ipWithSubnet, gw});
    return true;
}

int RouterMac::routeAddList(const QString &gw, const QStringList &ips)
{
    int cnt = 0;
    for (const QString &ip: ips) {
        if (routeAdd(ip, gw)) cnt++;
    }
    return cnt;
}

bool RouterMac::clearSavedRoutes()
{
    int cnt = 0;
    for (const Route &r: m_addedRoutes) {
        if (routeDelete(r.dst, r.gw)) cnt++;
    }
    bool ret = (cnt == m_addedRoutes.count());
    m_addedRoutes.clear();
    return ret;
}

bool RouterMac::routeDelete(const QString &ipWithSubnet, const QString &gw)
{
    QString ip = NetworkUtilities::ipAddressFromIpWithSubnet(ipWithSubnet);
    QString mask = NetworkUtilities::netMaskFromIpWithSubnet(ipWithSubnet);

#ifdef MZ_DEBUG
    qDebug().noquote() << "RouterMac::routeDelete: " << ipWithSubnet << gw;
#endif

    if (!NetworkUtilities::checkIPv4Format(ip) || !NetworkUtilities::checkIPv4Format(gw)) {
        qCritical().noquote() << "Critical, trying to remove invalid route: " << ip << gw;
        return false;
    }

    if (ipWithSubnet == "0.0.0.0/0") {
        qDebug().noquote() << "Warning, trying to remove default route, skipping: " << ip << gw;
        return true;
    }

    const Route *added = nullptr;
    for (const Route &route : m_addedRoutes) {
        if (route.dst == ipWithSubnet && route.gw == gw) {
            added = &route;
            break;
        }
    }
    if (added && !added->iface.isEmpty()) {
        const QStringList args = {"-n", "delete", "-net", ip, "-netmask", mask};
        if (added->scopedAdded)
            QProcess::execute("/sbin/route", args + QStringList{"-ifscope", added->iface});
        if (added->directAdded)
            QProcess::execute("/sbin/route", args);
    } else if (mask == "255.255.255.255") {
        runRoute(QString("route delete -host %1 %2").arg(ip, gw));
    } else {
        runRoute(QString("route delete -net %1 %2 %3").arg(ip, gw, mask));
    }
    return true;
}

bool RouterMac::routeDeleteList(const QString &gw, const QStringList &ips)
{
    int cnt = 0;
    for (const QString &ip: ips) {
        if (routeDelete(ip, gw)) cnt++;
    }
    return cnt;
}

bool RouterMac::createTun(const QString &dev, const QString &subnet) {
    qDebug().noquote() << "createTun start";

    QProcess process;
    QStringList commands;

    commands << "ifconfig" << dev << "inet" << subnet << subnet << "up";
    process.start("sudo", commands);
    if (!process.waitForStarted(1000))
    {
        qDebug().noquote() << "Could not start activate tun device!\n";
        return false;
    }
    else if (!process.waitForFinished(2000))
    {
        qDebug().noquote() << "Could not activate tun device!\n";
        return false;
    }
    commands.clear();

    return true;
}

bool RouterMac::updateResolvers(const QString& ifname, const QList<QHostAddress>& resolvers)
{
    return m_dnsUtil->updateResolvers(ifname, resolvers);
}

bool RouterMac::restoreResolvers() {
    return m_dnsUtil->restoreResolvers();
}

bool RouterMac::routeAddXray(const QString& ifname, const QString& gateway)
{
    if (ifname.isEmpty() || gateway.isEmpty()) {
        qWarning().noquote() << "routeAddXray: invalid iface/gateway:" << ifname << gateway;
        return false;
    }

    QString cmd = QString("route add -net 0.0.0.0/1 %1 -ifscope %2").arg(gateway).arg(ifname);
    QStringList parts = cmd.split(" ");

    int argc = parts.size();
    char **argv = new char*[argc];
    for (int i = 0; i < argc; i++) {
        argv[i] = new char[parts.at(i).toStdString().length() + 1];
        strcpy(argv[i], parts.at(i).toStdString().c_str());
    }
    mainRouteIface(argc, argv);
    for (int i = 0; i < argc; i++) {
        delete [] argv[i];
    }
    delete[] argv;

    cmd = QString("route add -net 128.0.0.0/1 %1 -ifscope %2").arg(gateway).arg(ifname);
    parts = cmd.split(" ");

    argc = parts.size();
    argv = new char*[argc];
    for (int i = 0; i < argc; i++) {
        argv[i] = new char[parts.at(i).toStdString().length() + 1];
        strcpy(argv[i], parts.at(i).toStdString().c_str());
    }
    mainRouteIface(argc, argv);
    for (int i = 0; i < argc; i++) {
        delete [] argv[i];
    }
    delete[] argv;

    qDebug().noquote() << "Installed xray routes via" << gateway << "on" << ifname;
    return true;
}

bool RouterMac::routeDeleteXray(const QString& ifname, const QString& gateway)
{
    if (ifname.isEmpty()) {
        return false;
    }

    QString cmd;
    if (!gateway.isEmpty()) {
        cmd = QString("route delete -net 0.0.0.0/1 %1 -ifscope %2").arg(gateway).arg(ifname);
    } else {
        cmd = QString("route delete -net 0.0.0.0/1 -ifscope %1").arg(ifname);
    }
    QStringList parts = cmd.split(" ");

    int argc = parts.size();
    char **argv = new char*[argc];
    for (int i = 0; i < argc; i++) {
        argv[i] = new char[parts.at(i).toStdString().length() + 1];
        strcpy(argv[i], parts.at(i).toStdString().c_str());
    }
    mainRouteIface(argc, argv);
    for (int i = 0; i < argc; i++) {
        delete [] argv[i];
    }
    delete[] argv;

    if (!gateway.isEmpty()) {
        cmd = QString("route delete -net 128.0.0.0/1 %1 -ifscope %2").arg(gateway).arg(ifname);
    } else {
        cmd = QString("route delete -net 128.0.0.0/1 -ifscope %1").arg(ifname);
    }
    parts = cmd.split(" ");

    argc = parts.size();
    argv = new char*[argc];
    for (int i = 0; i < argc; i++) {
        argv[i] = new char[parts.at(i).toStdString().length() + 1];
        strcpy(argv[i], parts.at(i).toStdString().c_str());
    }
    mainRouteIface(argc, argv);
    for (int i = 0; i < argc; i++) {
        delete [] argv[i];
    }
    delete[] argv;

    qDebug().noquote() << "Removed xray routes on" << ifname;
    return true;
}

bool RouterMac::deleteTun(const QString &dev)
{
    qDebug().noquote() << "deleteTun start";

    return true;
}

bool RouterMac::flushDns()
{
    // sudo killall -HUP mDNSResponder
    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);

    p.start("killall", QStringList() << "-HUP" << "mDNSResponder");
    p.waitForFinished();
    
    qDebug().noquote() << "OUTPUT killall -HUP mDNSResponder: " + p.readAll();
    return true;
}
