#include <QDebug>
#include <QDesktopServices>
#include <QFile>
#include <QHostInfo>

#include "SitesLogic.h"
#include "VpnLogic.h"
#include "utilities.h"
#include "vpnconnection.h"
#include <functional>

#include "../uilogic.h"
#include "../models/sites_model.h"

SitesLogic::SitesLogic(UiLogic *logic, QObject *parent):
    PageLogicBase(logic, parent),
    m_labelSitesAddCustomText{},
    m_tableViewSitesModel{nullptr},
    m_lineEditSitesAddCustomText{}
{
    sitesModels.insert(Settings::VpnOnlyForwardSites, new SitesModel(m_settings, Settings::VpnOnlyForwardSites));
    sitesModels.insert(Settings::VpnAllExceptSites, new SitesModel(m_settings, Settings::VpnAllExceptSites));
}

void SitesLogic::onUpdatePage()
{
    Settings::RouteMode m = m_settings->routeMode();
    if (m == Settings::VpnAllSites) return;

    if (m == Settings::VpnOnlyForwardSites) {
        set_labelSitesAddCustomText(tr("These sites will be opened using VPN"));
    }
    if (m == Settings::VpnAllExceptSites) {
        set_labelSitesAddCustomText(tr("These sites will be excepted from VPN"));
    }

    set_tableViewSitesModel(sitesModels.value(m));
    sitesModels.value(m)->resetCache();
}

void SitesLogic::onPushButtonAddCustomSitesClicked()
{
    if (uiLogic()->pageLogic<VpnLogic>()->radioButtonVpnModeAllSitesChecked()) {
        return;
    }
    Settings::RouteMode mode = m_settings->routeMode();

    QString newSite = lineEditSitesAddCustomText();

    if (newSite.isEmpty()) return;
    if (!newSite.contains(".")) return;

    if (!Utils::ipAddressWithSubnetRegExp().exactMatch(newSite)) {
        // get domain name if it present
        newSite.replace("https://", "");
        newSite.replace("http://", "");
        newSite.replace("ftp://", "");

        newSite = newSite.split("/", Qt::SkipEmptyParts).first();
    }

    const auto &cbProcess = [this, mode](const QString &newSite, const QString &ip) {
        m_settings->addVpnSite(mode, newSite, ip);

        if (!ip.isEmpty()) {
            QMetaObject::invokeMethod(uiLogic()->m_vpnConnection, "addRoutes",
                                      Qt::QueuedConnection,
                                      Q_ARG(QStringList, QStringList() << ip));
        }
        else if (Utils::ipAddressWithSubnetRegExp().exactMatch(newSite)) {
            QMetaObject::invokeMethod(uiLogic()->m_vpnConnection, "addRoutes",
                                      Qt::QueuedConnection,
                                      Q_ARG(QStringList, QStringList() << newSite));
        }

        QMetaObject::invokeMethod(uiLogic()->m_vpnConnection, "flushDns",
                                  Qt::QueuedConnection);

        onUpdatePage();
    };

    const auto &cbResolv = [this, cbProcess](const QHostInfo &hostInfo){
        const QList<QHostAddress> &addresses = hostInfo.addresses();
        QString ipv4Addr;
        for (const QHostAddress &addr: hostInfo.addresses()) {
            if (addr.protocol() == QAbstractSocket::NetworkLayerProtocol::IPv4Protocol) {
                cbProcess(hostInfo.hostName(), addr.toString());
                break;
            }
        }
    };

    set_lineEditSitesAddCustomText("");

    if (Utils::ipAddressWithSubnetRegExp().exactMatch(newSite)) {
        cbProcess(newSite, "");
        return;
    }
    else {
        cbProcess(newSite, "");
        onUpdatePage();
        QHostInfo::lookupHost(newSite, this, cbResolv);
    }
}

void SitesLogic::onPushButtonSitesDeleteClicked(QStringList items)
{
    Settings::RouteMode mode = m_settings->routeMode();

    auto siteModel = qobject_cast<SitesModel*> (tableViewSitesModel());
    if (!siteModel || items.isEmpty()) {
        return;
    }

    QStringList sites;
    QStringList ips;

    for (const QString &s: items) {
        bool ok;
        int row = s.toInt(&ok);
        if (!ok || row < 0 || row >= siteModel->rowCount()) return;
        sites.append(siteModel->data(row, 0).toString());

        if (uiLogic()->m_vpnConnection && uiLogic()->m_vpnConnection->connectionState() == VpnProtocol::Connected) {
            ips.append(siteModel->data(row, 1).toString());
        }
    }

    m_settings->removeVpnSites(mode, sites);

    QMetaObject::invokeMethod(uiLogic()->m_vpnConnection, "deleteRoutes",
                              Qt::QueuedConnection,
                              Q_ARG(QStringList, ips));

    QMetaObject::invokeMethod(uiLogic()->m_vpnConnection, "flushDns",
                              Qt::QueuedConnection);

    onUpdatePage();
}

void SitesLogic::onPushButtonSitesImportClicked(const QString& fileName)
{
    QFile file(QUrl{fileName}.toLocalFile());
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Can't open file " << QUrl{fileName}.toLocalFile();
        return;
    }

    Settings::RouteMode mode = m_settings->routeMode();

    QStringList ips;
    QMap<QString, QString> sites;

    while (!file.atEnd()) {
        QString line = file.readLine();
        QStringList line_ips;
        QStringList line_sites;

        int posDomain = 0;
        QRegExp domainRx = Utils::domainRegExp();
        while ((posDomain = domainRx.indexIn(line, posDomain)) != -1) {
            line_sites.append(domainRx.cap(0));
            posDomain += domainRx.matchedLength();
        }

        int posIp = 0;
        QRegExp ipRx = Utils::ipAddressWithSubnetRegExp();
        while ((posIp = ipRx.indexIn(line, posIp)) != -1) {
            line_ips.append(ipRx.cap(0));
            posIp += ipRx.matchedLength();
        }

        // domain regex cover ip regex, so remove ips from sites
        for (const QString& ip: line_ips) {
            line_sites.removeAll(ip);
        }

        if (line_sites.size() == 1 && line_ips.size() == 1) {
            sites.insert(line_sites.at(0), line_ips.at(0));
        }
        else if (line_sites.size() > 0 && line_ips.size() == 0) {
            for (const QString& site: line_sites) {
                sites.insert(site, "");
            }
        }
        else {
            for (const QString& site: line_sites) {
                sites.insert(site, "");
            }
            for (const QString& ip: line_ips) {
                ips.append(ip);
            }
        }

    }

    m_settings->addVpnIps(mode, ips);
    m_settings->addVpnSites(mode, sites);

    QMetaObject::invokeMethod(uiLogic()->m_vpnConnection, "addRoutes",
                              Qt::QueuedConnection,
                              Q_ARG(QStringList, ips));

    QMetaObject::invokeMethod(uiLogic()->m_vpnConnection, "flushDns",
                              Qt::QueuedConnection);

    onUpdatePage();
}

void SitesLogic::onPushButtonSitesExportClicked()
{
    Settings::RouteMode mode = m_settings->routeMode();

    QVariantMap sites = m_settings->vpnSites(mode);

    QString data;
    for (auto s : sites.keys()) {
        data += s + "\t" + sites.value(s).toString() + "\n";
    }
    uiLogic()->saveTextFile("Export Sites", "sites.txt", ".txt", data);
}

