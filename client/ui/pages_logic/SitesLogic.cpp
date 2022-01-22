#include <QDebug>
#include <QDesktopServices>
#include <QFile>
#include <QHostInfo>

#include "SitesLogic.h"
#include "VpnLogic.h"
#include "utils.h"
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
    sitesModels.insert(Settings::VpnOnlyForwardSites, new SitesModel(Settings::VpnOnlyForwardSites));
    sitesModels.insert(Settings::VpnAllExceptSites, new SitesModel(Settings::VpnAllExceptSites));
}

void SitesLogic::onUpdatePage()
{
    Settings::RouteMode m = m_settings.routeMode();
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
    if (uiLogic()->vpnLogic()->radioButtonVpnModeAllSitesChecked()) {
        return;
    }
    Settings::RouteMode mode = m_settings.routeMode();

    QString newSite = lineEditSitesAddCustomText();

    if (newSite.isEmpty()) return;
    if (!newSite.contains(".")) return;

    if (!Utils::ipAddressWithSubnetRegExp().exactMatch(newSite)) {
        // get domain name if it present
        newSite.replace("https://", "");
        newSite.replace("http://", "");
        newSite.replace("ftp://", "");

        newSite = newSite.split("/", QString::SkipEmptyParts).first();
    }

    const auto &cbProcess = [this, mode](const QString &newSite, const QString &ip) {
        m_settings.addVpnSite(mode, newSite, ip);

        if (!ip.isEmpty()) {
            uiLogic()->m_vpnConnection->addRoutes(QStringList() << ip);
            uiLogic()->m_vpnConnection->flushDns();
        }
        else if (Utils::ipAddressWithSubnetRegExp().exactMatch(newSite)) {
            uiLogic()->m_vpnConnection->addRoutes(QStringList() << newSite);
            uiLogic()->m_vpnConnection->flushDns();
        }

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
    Settings::RouteMode mode = m_settings.routeMode();

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

        if (uiLogic()->m_vpnConnection->connectionState() == VpnProtocol::Connected) {
            ips.append(siteModel->data(row, 1).toString());
        }
    }

    m_settings.removeVpnSites(mode, sites);

    if (uiLogic()->m_vpnConnection->connectionState() == VpnProtocol::Connected) {
        uiLogic()->m_vpnConnection->deleteRoutes(ips);
        uiLogic()->m_vpnConnection->flushDns();
    }

    onUpdatePage();
}

void SitesLogic::onPushButtonSitesImportClicked(const QString& fileName)
{
    QFile file(QUrl{fileName}.toLocalFile());
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Can't open file " << QUrl{fileName}.toLocalFile();
        return;
    }

    Settings::RouteMode mode = m_settings.routeMode();

    QStringList ips;
    while (!file.atEnd()) {
        QString line = file.readLine();

        int pos = 0;
        QRegExp rx = Utils::ipAddressWithSubnetRegExp();
        while ((pos = rx.indexIn(line, pos)) != -1) {
            ips << rx.cap(0);
            pos += rx.matchedLength();
        }
    }

    m_settings.addVpnIps(mode, ips);

    uiLogic()->m_vpnConnection->addRoutes(QStringList() << ips);
    uiLogic()->m_vpnConnection->flushDns();

    onUpdatePage();
}

void SitesLogic::onPushButtonSitesExportClicked()
{
    Settings::RouteMode mode = m_settings.routeMode();

    QVariantMap sites = m_settings.vpnSites(mode);

    QString data;
    for (auto s : sites.keys()) {
        data += s + "\t" + sites.value(s).toString() + "\n";
    }
    uiLogic()->saveTextFile("Sites", ".txt", data);
}

