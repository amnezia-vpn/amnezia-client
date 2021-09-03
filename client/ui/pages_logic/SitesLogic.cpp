#include <QDebug>
#include <QDesktopServices>
#include <QHostInfo>


#include "SitesLogic.h"
#include "utils.h"
#include "vpnconnection.h"
#include <functional>

#include "../uilogic.h"
#include "../sites_model.h"


using namespace amnezia;
using namespace PageEnumNS;


SitesLogic::SitesLogic(UiLogic *uiLogic, QObject *parent):
    QObject(parent),
    m_uiLogic(uiLogic),
    m_labelSitesAddCustomText{},
    m_tableViewSitesModel{nullptr},
    m_lineEditSitesAddCustomText{}
{
    sitesModels.insert(Settings::VpnOnlyForwardSites, new SitesModel(Settings::VpnOnlyForwardSites));
    sitesModels.insert(Settings::VpnAllExceptSites, new SitesModel(Settings::VpnAllExceptSites));
}

void SitesLogic::updateSitesPage()
{
    Settings::RouteMode m = m_settings.routeMode();
    if (m == Settings::VpnAllSites) return;

    if (m == Settings::VpnOnlyForwardSites) {
        setLabelSitesAddCustomText(tr("These sites will be opened using VPN"));
    }
    if (m == Settings::VpnAllExceptSites) {
        setLabelSitesAddCustomText(tr("These sites will be excepted from VPN"));
    }

    setTableViewSitesModel(sitesModels.value(m));
    sitesModels.value(m)->resetCache();
}

QString SitesLogic::getLabelSitesAddCustomText() const
{
    return m_labelSitesAddCustomText;
}

void SitesLogic::setLabelSitesAddCustomText(const QString &labelSitesAddCustomText)
{
    if (m_labelSitesAddCustomText != labelSitesAddCustomText) {
        m_labelSitesAddCustomText = labelSitesAddCustomText;
        emit labelSitesAddCustomTextChanged();
    }
}

QObject* SitesLogic::getTableViewSitesModel() const
{
    return m_tableViewSitesModel;
}

void SitesLogic::setTableViewSitesModel(QObject* tableViewSitesModel)
{
    if (m_tableViewSitesModel != tableViewSitesModel) {
        m_tableViewSitesModel = tableViewSitesModel;
        emit tableViewSitesModelChanged();
    }
}

QString SitesLogic::getLineEditSitesAddCustomText() const
{
    return m_lineEditSitesAddCustomText;
}

void SitesLogic::setLineEditSitesAddCustomText(const QString &lineEditSitesAddCustomText)
{
    if (m_lineEditSitesAddCustomText != lineEditSitesAddCustomText) {
        m_lineEditSitesAddCustomText = lineEditSitesAddCustomText;
        emit lineEditSitesAddCustomTextChanged();
    }
}

void SitesLogic::onPushButtonAddCustomSitesClicked()
{
    if (m_uiLogic->getRadioButtonVpnModeAllSitesChecked()) {
        return;
    }
    Settings::RouteMode mode = m_settings.routeMode();

    QString newSite = getLineEditSitesAddCustomText();

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
            m_uiLogic->m_vpnConnection->addRoutes(QStringList() << ip);
            m_uiLogic->m_vpnConnection->flushDns();
        }
        else if (Utils::ipAddressWithSubnetRegExp().exactMatch(newSite)) {
            m_uiLogic->m_vpnConnection->addRoutes(QStringList() << newSite);
            m_uiLogic->m_vpnConnection->flushDns();
        }

        updateSitesPage();
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

    setLineEditSitesAddCustomText("");

    if (Utils::ipAddressWithSubnetRegExp().exactMatch(newSite)) {
        cbProcess(newSite, "");
        return;
    }
    else {
        cbProcess(newSite, "");
        updateSitesPage();
        QHostInfo::lookupHost(newSite, this, cbResolv);
    }
}

void SitesLogic::onPushButtonSitesDeleteClicked(int row)
{
    Settings::RouteMode mode = m_settings.routeMode();

    auto siteModel = qobject_cast<SitesModel*> (getTableViewSitesModel());
    if (!siteModel) {
        return;
    }
    if (row < 0 || row >= siteModel->rowCount()) {
        return;
    }

    {
        QStringList sites;
        sites.append(siteModel->data(row, 0).toString());
        m_settings.removeVpnSites(mode, sites);
    }

    if (m_uiLogic->m_vpnConnection->connectionState() == VpnProtocol::Connected) {
        QStringList ips;
        ips.append(siteModel->data(row, 1).toString());
        m_uiLogic->m_vpnConnection->deleteRoutes(ips);
        m_uiLogic->m_vpnConnection->flushDns();
    }

    updateSitesPage();
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

    m_uiLogic->m_vpnConnection->addRoutes(QStringList() << ips);
    m_uiLogic->m_vpnConnection->flushDns();

    updateSitesPage();
}

