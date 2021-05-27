#include "router_mac.h"
#include "helper_route_mac.h"

#include <QProcess>

RouterMac &RouterMac::Instance()
{
    static RouterMac s;
    return s;
}

bool RouterMac::routeAdd(const QString &ip, const QString &gw)
{
    int argc = 5;
    char **argv = new char*[argc];

    argv[0] = new char[std::string("route").length() + 1];
    strcpy(argv[0], std::string("route").c_str());

    argv[1] = new char[std::string("add").length() + 1];
    strcpy(argv[1], std::string("add").c_str());

    argv[2] = new char[6];
    strcpy(argv[2], std::string("-host").c_str());

    argv[3] = new char[ip.toStdString().length() + 1];
    strcpy(argv[3], ip.toStdString().c_str());

    argv[4] = new char[gw.toStdString().length() + 1];
    strcpy(argv[4], gw.toStdString().c_str());

    mainRouteIface(argc, argv);

    for (int i = 0; i < argc; i++) {
        delete [] argv[i];
    }
    delete[] argv;
    return true;

//    //  route add -host ip gw
//    QProcess p;
//    p.setProcessChannelMode(QProcess::MergedChannels);

//    p.start("route", QStringList() << "add" << "-host" << ip << gw);
//    p.waitForFinished();
//    qDebug().noquote() << "OUTPUT routeAdd: " + p.readAll();
//    bool ok = (p.exitCode() == 0);
//    if (ok) {
//        m_addedRoutes.append(ip);
//    }
//    return ok;
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

bool RouterMac::routeDelete(const QString &ip, const QString &gw)
{
    if (ip == "0.0.0.0") {
        qDebug().noquote() << "Warning, trying to remove default route, skipping: " << ip << gw;
        return true;
    }

    //  route delete ip gw
    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);

    p.start("route", QStringList() << "delete" << ip);
    p.waitForFinished();
    // skipping gw
    qDebug().noquote() << "routeDelete, skipping gw`: " << ip;
    qDebug().noquote() << "OUTPUT routeDelete: " << p.readAll();

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
