#include "router_win.h"

#include <QProcess>

RouterWin &RouterWin::Instance()
{
    static RouterWin s;
    return s;
}

bool RouterWin::routeAdd(const QString &ip, const QString &gw, QString mask)
{
    qDebug().noquote() << QString("ROUTE ADD: IP:%1 %2 GW %3")
                          .arg(ip)
                          .arg(mask)
                          .arg(gw);

    if (mask == "") {
        mask = "255.255.255.255";
        if (ip.endsWith(".0")) mask = "255.255.255.0";
        if (ip.endsWith(".0.0")) mask = "255.255.0.0";
        if (ip.endsWith(".0.0.0")) mask = "255.0.0.0";
    }


    PMIB_IPFORWARDTABLE pIpForwardTable = NULL;
    MIB_IPFORWARDROW ipfrow;
    DWORD dwSize = 0;
    BOOL bOrder = FALSE;
    DWORD dwStatus = 0;


    // Find out how big our buffer needs to be.
    dwStatus = GetIpForwardTable(pIpForwardTable, &dwSize, bOrder);
    if (dwStatus == ERROR_INSUFFICIENT_BUFFER) {
        // Allocate the memory for the table
        if (!(pIpForwardTable = (PMIB_IPFORWARDTABLE) malloc(dwSize))) {
            qDebug() << "Malloc failed. Out of memory.";
            return false;
        }
        // Now get the table.
        dwStatus = GetIpForwardTable(pIpForwardTable, &dwSize, bOrder);
    }


    if (dwStatus != ERROR_SUCCESS) {
        qDebug() << "getIpForwardTable failed.";
        if (pIpForwardTable)
            free(pIpForwardTable);
        return false;
    }

    // Set iface for route
    IPAddr  dwGwAddr = inet_addr(gw.toStdString().c_str());
    if (GetBestInterface(dwGwAddr, &ipfrow.dwForwardIfIndex) != NO_ERROR) {
        qDebug() << "Router::routeAdd : GetBestInterface failed";
        return false;
    }

    // address
    ipfrow.dwForwardDest = inet_addr(ip.toStdString().c_str());

    // mask
    in_addr maskAddr;
    inet_pton(AF_INET, mask.toStdString().c_str(), &maskAddr);
    ipfrow.dwForwardMask = maskAddr.S_un.S_addr;

    // Get TAP iface metric to set it for new routes
    MIB_IPINTERFACE_ROW tap_iface;
    InitializeIpInterfaceEntry(&tap_iface);
    tap_iface.InterfaceIndex = ipfrow.dwForwardIfIndex;
    tap_iface.Family = AF_INET;
    dwStatus  = GetIpInterfaceEntry(&tap_iface);
    if (dwStatus == NO_ERROR){
        ipfrow.dwForwardMetric1 = tap_iface.Metric;
    }
    else {
        qDebug() << "Router::routeAdd: failed GetIpInterfaceEntry(), Error:" << dwStatus;
        ipfrow.dwForwardMetric1 = 256;
    }
    ipfrow.dwForwardMetric2 = 0;
    ipfrow.dwForwardMetric3 = 0;
    ipfrow.dwForwardMetric4 = 0;
    ipfrow.dwForwardMetric5 = 0;

    ipfrow.dwForwardAge = 0;

    ipfrow.dwForwardNextHop = inet_addr(gw.toStdString().c_str());
    ipfrow.dwForwardType = 4;	/* XXX - next hop != final dest */
    ipfrow.dwForwardProto = 3;	/* XXX - MIB_PROTO_NETMGMT */


    dwStatus  = CreateIpForwardEntry(&ipfrow);
    if (dwStatus == NO_ERROR){
        ipForwardRows.append(ipfrow);
        //qDebug() <<  "Gateway changed successfully";
    }
    else {
        qDebug() << "Router::routeAdd: failed CreateIpForwardEntry()";
        qDebug() << "Error: " << dwStatus;
    }

    // Free resources
    if (pIpForwardTable)
        free(pIpForwardTable);

    return (dwStatus == NO_ERROR);
}

int RouterWin::routeAddList(const QString &gw, const QStringList &ips)
{
    qDebug().noquote() << QString("ROUTE ADD List: IPs size:%1, GW: %2")
                          .arg(ips.size())
                          .arg(gw);

    qDebug().noquote() << QString("ROUTE ADD List: IPs:\n%1")
                          .arg(ips.join("\n"));



    PMIB_IPFORWARDTABLE pIpForwardTable = NULL;
    DWORD dwSize = 0;
    BOOL bOrder = FALSE;
    DWORD dwStatus = 0;


    // Find out how big our buffer needs to be.
    dwStatus = GetIpForwardTable(pIpForwardTable, &dwSize, bOrder);
    if (dwStatus == ERROR_INSUFFICIENT_BUFFER) {
        // Allocate the memory for the table
        if (!(pIpForwardTable = (PMIB_IPFORWARDTABLE) malloc(dwSize))) {
            qDebug() << "Malloc failed. Out of memory.";
            return 0;
        }
        // Now get the table.
        dwStatus = GetIpForwardTable(pIpForwardTable, &dwSize, bOrder);
    }


    if (dwStatus != ERROR_SUCCESS) {
        qDebug() << "getIpForwardTable failed.";
        if (pIpForwardTable)
            free(pIpForwardTable);
        return 0;
    }


    int success_count = 0;

    QString mask;

    MIB_IPFORWARDROW ipfrow;


    ipfrow.dwForwardPolicy = 0;
    ipfrow.dwForwardAge = 0;

    ipfrow.dwForwardNextHop = inet_addr(gw.toStdString().c_str());
    ipfrow.dwForwardType = 4;	/* XXX - next hop != final dest */
    ipfrow.dwForwardProto = 3;	/* XXX - MIB_PROTO_NETMGMT */


    // Set iface for route
    IPAddr  dwGwAddr = inet_addr(gw.toStdString().c_str());
    if (GetBestInterface(dwGwAddr, &ipfrow.dwForwardIfIndex) != NO_ERROR) {
        qDebug() << "Router::routeAddList : GetBestInterface failed";
        return false;
    }

    // Get TAP iface metric to set it for new routes
    MIB_IPINTERFACE_ROW tap_iface;
    InitializeIpInterfaceEntry(&tap_iface);
    tap_iface.InterfaceIndex = ipfrow.dwForwardIfIndex;
    tap_iface.Family = AF_INET;
    dwStatus  = GetIpInterfaceEntry(&tap_iface);
    if (dwStatus == NO_ERROR){
        ipfrow.dwForwardMetric1 = tap_iface.Metric;
    }
    else {
        qDebug() << "Router::routeAddList: failed GetIpInterfaceEntry(), Error:" << dwStatus;
        ipfrow.dwForwardMetric1 = 256;
    }
    ipfrow.dwForwardMetric2 = 0;
    ipfrow.dwForwardMetric3 = 0;
    ipfrow.dwForwardMetric4 = 0;
    ipfrow.dwForwardMetric5 = 0;

    for (int i = 0; i < ips.size(); ++i) {
        QString ip = ips.at(i);
        if (ip.isEmpty()) continue;

        mask = "255.255.255.255";
        if (ip.endsWith(".0")) mask = "255.255.255.0";
        if (ip.endsWith(".0.0")) mask = "255.255.0.0";
        if (ip.endsWith(".0.0.0")) mask = "255.0.0.0";

        // address
        ipfrow.dwForwardDest = inet_addr(ip.toStdString().c_str());


        // mask
        in_addr maskAddr;
        inet_pton(AF_INET, mask.toStdString().c_str(), &maskAddr);
        ipfrow.dwForwardMask = maskAddr.S_un.S_addr;

        dwStatus  = CreateIpForwardEntry(&ipfrow);
        if (dwStatus == NO_ERROR){
            ipForwardRows.append(ipfrow);
            //qDebug() <<  "Gateway changed successfully";
        }
        else {
            qDebug() << "Router::routeAdd: failed CreateIpForwardEntry(), Error:" << ip << dwStatus;
        }

        if (dwStatus == NO_ERROR) success_count++;
    }


    // Free resources
    if (pIpForwardTable)
        free(pIpForwardTable);

    qDebug() << "Router::routeAddList finished, success: " << success_count << "/" << ips.size();
    return success_count;
}

bool RouterWin::clearSavedRoutes()
{
    if (ipForwardRows.isEmpty()) return true;

    qDebug() << "forward rows size:" << ipForwardRows.size();

    // Declare and initialize variables
    PMIB_IPFORWARDTABLE pIpForwardTable = NULL;
    DWORD dwSize = 0;
    BOOL bOrder = FALSE;
    DWORD dwStatus = 0;

    // Find out how big our buffer needs to be.
    dwStatus = GetIpForwardTable(pIpForwardTable, &dwSize, bOrder);
    if (dwStatus == ERROR_INSUFFICIENT_BUFFER) {
        // Allocate the memory for the table
        if (!(pIpForwardTable = (PMIB_IPFORWARDTABLE) malloc(dwSize))) {
            qDebug() << "Router::clearSavedRoutes : Malloc failed. Out of memory";
            return false;
        }
        // Now get the table.
        dwStatus = GetIpForwardTable(pIpForwardTable, &dwSize, bOrder);
    }

    if (dwStatus != ERROR_SUCCESS) {
        qDebug() << "Router::clearSavedRoutes : getIpForwardTable failed";
        if (pIpForwardTable)
            free(pIpForwardTable);
        return false;
    }

    int removed_count = 0;
    for (int i = 0; i < ipForwardRows.size(); ++i) {
        dwStatus = DeleteIpForwardEntry(&ipForwardRows[i]);

        if (dwStatus != ERROR_SUCCESS) {
            qDebug() << "Router::clearSavedRoutes : Could not delete old row" << i;
        }
        else  removed_count++;
    }

    if (pIpForwardTable)
        free(pIpForwardTable);

    qDebug() << "Router::clearSavedRoutes : removed routes:" << removed_count << "of" << ipForwardRows.size();
    ipForwardRows.clear();

    return true;
}

bool RouterWin:routeDelete(const QString &ip)
{
    qDebug().noquote() << QString("ROUTE DELETE, IP: %1").arg(ip);

    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);
    QString command = QString("route delete %1")
            .arg(ip);

    p.start(command);
    p.waitForFinished();
    qDebug().noquote() << "OUTPUT route delete: " + p.readAll();

    return true;
}

void RouterWin::flushDns()
{
    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);
    QString command = QString("ipconfig /flushdns");

    p.start(command);
    p.waitForFinished();
    //qDebug().noquote() << "OUTPUT ipconfig /flushdns: " + p.readAll();
}
