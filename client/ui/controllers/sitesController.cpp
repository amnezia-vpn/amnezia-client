#include "sitesController.h"

#include <QHostInfo>

#include "utilities.h"

SitesController::SitesController(const std::shared_ptr<Settings> &settings,
                                 const QSharedPointer<VpnConnection> &vpnConnection,
                                 const QSharedPointer<SitesModel> &sitesModel, QObject *parent)
    : QObject(parent), m_settings(settings), m_vpnConnection(vpnConnection), m_sitesModel(sitesModel)
{
}

void SitesController::addSite(QString hostname)
{
    if (hostname.isEmpty()) {
        return;
    }

    if (!hostname.contains(".")) {
        emit errorOccurred(tr("Hostname not look like ip adress or domain name"));
        return;
    }

    if (!Utils::ipAddressWithSubnetRegExp().exactMatch(hostname)) {
        // get domain name if it present
        hostname.replace("https://", "");
        hostname.replace("http://", "");
        hostname.replace("ftp://", "");

        hostname = hostname.split("/", Qt::SkipEmptyParts).first();
    }

    const auto &processSite = [this](const QString &hostname, const QString &ip) {
        m_sitesModel->addSite(hostname, ip);

        if (!ip.isEmpty()) {
            m_vpnConnection->addRoutes(QStringList() << ip);
            m_vpnConnection->flushDns();
        } else if (Utils::ipAddressWithSubnetRegExp().exactMatch(hostname)) {
            m_vpnConnection->addRoutes(QStringList() << hostname);
            m_vpnConnection->flushDns();
        }
    };

    const auto &resolveCallback = [this, processSite](const QHostInfo &hostInfo) {
        const QList<QHostAddress> &addresses = hostInfo.addresses();
        for (const QHostAddress &addr : hostInfo.addresses()) {
            if (addr.protocol() == QAbstractSocket::NetworkLayerProtocol::IPv4Protocol) {
                processSite(hostInfo.hostName(), addr.toString());
                break;
            }
        }
    };

    if (Utils::ipAddressWithSubnetRegExp().exactMatch(hostname)) {
        processSite(hostname, "");
    } else {
        processSite(hostname, "");
        QHostInfo::lookupHost(hostname, this, resolveCallback);
    }

    emit finished(tr("New site added: ") + hostname);
}

void SitesController::removeSite(int index)
{
    auto modelIndex = m_sitesModel->index(index);
    auto hostname = m_sitesModel->data(modelIndex, SitesModel::Roles::UrlRole).toString();
    m_sitesModel->removeSite(modelIndex);

    emit finished(tr("Site removed: ") + hostname);
}

void SitesController::importSites(bool replaceExisting)
{
    QString fileName = Utils::getFileName(Q_NULLPTR, tr("Open sites file"),
                                          QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), "*.json");

    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        emit errorOccurred(tr("Can't open file: ") + fileName);
        return;
    }

    QByteArray jsonData = file.readAll();
    QJsonDocument jsonDocument = QJsonDocument::fromJson(jsonData);
    if (jsonDocument.isNull()) {
        emit errorOccurred(tr("Failed to parse JSON data from file: ") + fileName);
        return;
    }

    if (!jsonDocument.isArray()) {
        emit errorOccurred(tr("The JSON data is not an array in file: ") + fileName);
        return;
    }

    auto jsonArray = jsonDocument.array();
    QMap<QString, QString> sites;
    QStringList ips;

    for (auto jsonValue : jsonArray) {
        auto jsonObject = jsonValue.toObject();
        auto hostname = jsonObject.value("hostname").toString("");
        auto ip = jsonObject.value("ip").toString("");

        if (!hostname.contains(".") && !Utils::ipAddressWithSubnetRegExp().exactMatch(hostname)) {
            qDebug() << hostname << " not look like ip adress or domain name";
            continue;
        }

        if (ip.isEmpty()) {
            ips.append(hostname);
        } else {
            ips.append(ip);
        }
        sites.insert(hostname, ip);
    }

    m_sitesModel->addSites(sites, replaceExisting);

    m_vpnConnection->addRoutes(QStringList() << ips);
    m_vpnConnection->flushDns();

    emit finished(tr("Import completed"));
}

void SitesController::exportSites()
{
    auto sites = m_sitesModel->getCurrentSites();

    QJsonArray jsonArray;

    for (const auto &site : sites) {
        QJsonObject jsonObject { { "hostname", site.first }, { "ip", site.second } };
        jsonArray.append(jsonObject);
    }

    QJsonDocument jsonDocument(jsonArray);
    QByteArray jsonData = jsonDocument.toJson();

    Utils::saveFile(".json", tr("Export sites file"), "sites", jsonData);

    emit finished(tr("Export completed"));
}
