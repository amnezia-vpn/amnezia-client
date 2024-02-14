#include "router_mac.h"
#include "helper_route_mac.h"

#include <QProcess>
#include <QThread>
#include <utilities.h>

RouterMac &RouterMac::Instance()
{
    static RouterMac s;
    return s;
}

bool RouterMac::routeAdd(const QString &ipWithSubnet, const QString &gw)
{
    QString ip = Utils::ipAddressFromIpWithSubnet(ipWithSubnet);
    QString mask = Utils::netMaskFromIpWithSubnet(ipWithSubnet);

#ifdef MZ_DEBUG
    qDebug().noquote() << "RouterMac::routeAdd: " << ipWithSubnet << gw;
#endif

    if (!Utils::checkIPv4Format(ip) || !Utils::checkIPv4Format(gw)) {
        qCritical().noquote() << "Critical, trying to add invalid route: " << ip << gw;
        return false;
    }

    QString cmd;
    if (mask == "255.255.255.255") {
        cmd = QString("route add -host %1 %2").arg(ip).arg(gw);
    }
    else {
        cmd = QString("route add -net %1 %2 %3").arg(ip).arg(gw).arg(mask);
    }

    QStringList parts = cmd.split(" ");

    int argc = parts.size();
    char **argv = new char*[argc];

    for (int i = 0; i < argc; i++) {
        argv[i] = new char[parts.at(i).toStdString().length() + 1];
        strcpy(argv[i], parts.at(i).toStdString().c_str());
    }

    // TODO refactor
    mainRouteIface(argc, argv);
    m_addedRoutes.append({ipWithSubnet, gw});

    for (int i = 0; i < argc; i++) {
        delete [] argv[i];
    }
    delete[] argv;
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
    QString ip = Utils::ipAddressFromIpWithSubnet(ipWithSubnet);
    QString mask = Utils::netMaskFromIpWithSubnet(ipWithSubnet);

#ifdef MZ_DEBUG
    qDebug().noquote() << "RouterMac::routeDelete: " << ipWithSubnet << gw;
#endif

    if (!Utils::checkIPv4Format(ip) || !Utils::checkIPv4Format(gw)) {
        qCritical().noquote() << "Critical, trying to remove invalid route: " << ip << gw;
        return false;
    }

 //   if (ip == "0.0.0.0") {
 //       qDebug().noquote() << "Warning, trying to remove default route, skipping: " << ip << gw;
 //       return true;
 //   }

    QString cmd;
    if (mask == "255.255.255.255") {
        cmd = QString("route delete -host %1 %2").arg(ip).arg(gw);
    }
    else {
        cmd = QString("route delete -net %1 %2 %3").arg(ip).arg(gw).arg(mask);
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


bool RouterMac::deleteTun(const QString &dev)
{
    qDebug().noquote() << "deleteTun start";

    return true;
}

void RouterMac::flushDns()
{
    // sudo killall -HUP mDNSResponder
    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);

    p.start("killall", QStringList() << "-HUP" << "mDNSResponder");
    p.waitForFinished();
    qDebug().noquote() << "OUTPUT killall -HUP mDNSResponder: " + p.readAll();
}
