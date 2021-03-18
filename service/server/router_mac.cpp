#include "router_mac.h"

#include <QProcess>

RouterMac &RouterMac::Instance()
{
    static RouterMac s;
    return s;
}

bool RouterMac::routeAdd(const QString &ip, const QString &gw, QString mask)
{
    //  route add -host ip gw
    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);

    p.start("route", QStringList() << "add" << "-host" << ip << gw);
    p.waitForFinished();
    qDebug().noquote() << "OUTPUT routeAdd: " + p.readAll();
    bool ok = (p.exitCode() == 0);
    if (ok) {
        m_addedRoutes.append(ip);
    }
    return ok;
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
    // No need to delete routes after iface down
    return true;

//    int cnt = 0;
//    for (const QString &ip: m_addedRoutes) {
//        if (routeDelete(ip)) cnt++;
//    }
//    return (cnt == m_addedRoutes.count());
}

bool RouterMac::routeDelete(const QString &ip)
{
    //  route delete ip gw
    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);

    p.start("route", QStringList() << "delete" << ip);
    p.waitForFinished();
    qDebug().noquote() << "OUTPUT routeDelete: " + p.readAll();

    return p.exitCode() == 0;
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
