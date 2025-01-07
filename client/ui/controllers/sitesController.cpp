#include "sitesController.h"

#include <QFile>
#include <QHostInfo>
#include <QStandardPaths>

#include "systemController.h"
#include "core/networkUtilities.h"

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

    if (!NetworkUtilities::ipAddressWithSubnetRegExp().exactMatch(hostname)) {
        // get domain name if it present
        hostname.replace("https://", "");
        hostname.replace("http://", "");
        hostname.replace("ftp://", "");

        hostname = hostname.split("/", Qt::SkipEmptyParts).first();
    }

    const auto &processSite = [this](const QString &hostname, const QString &ip) {
        m_sitesModel->addSite(hostname, ip);

        if (!ip.isEmpty()) {
            QMetaObject::invokeMethod(m_vpnConnection.get(), "addRoutes", Qt::QueuedConnection,
                                      Q_ARG(QStringList, QStringList() << ip));
        } else if (NetworkUtilities::ipAddressWithSubnetRegExp().exactMatch(hostname)) {
            QMetaObject::invokeMethod(m_vpnConnection.get(), "addRoutes", Qt::QueuedConnection,
                                      Q_ARG(QStringList, QStringList() << hostname));
        }
        QMetaObject::invokeMethod(m_vpnConnection.get(), "flushDns", Qt::QueuedConnection);
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

    if (NetworkUtilities::ipAddressWithSubnetRegExp().exactMatch(hostname)) {
        processSite(hostname, "");
    } else {
        processSite(hostname, "");
        QHostInfo::lookupHost(hostname, this, resolveCallback);
    }

    emit finished(tr("New site added: %1").arg(hostname));
}

void SitesController::removeSite(int index)
{
    auto modelIndex = m_sitesModel->index(index);
    auto hostname = m_sitesModel->data(modelIndex, SitesModel::Roles::UrlRole).toString();
    m_sitesModel->removeSite(modelIndex);

    QMetaObject::invokeMethod(m_vpnConnection.get(), "deleteRoutes", Qt::QueuedConnection,
                              Q_ARG(QStringList, QStringList() << hostname));
    QMetaObject::invokeMethod(m_vpnConnection.get(), "flushDns", Qt::QueuedConnection);

    emit finished(tr("Site removed: %1").arg(hostname));
}

void SitesController::importSites(const QString &fileName, bool replaceExisting)
{
    QByteArray jsonData;
    if (!SystemController::readFile(fileName, jsonData)) {
        emit errorOccurred(tr("Can't open file: %1").arg(fileName));
        return;
    }

    QJsonDocument jsonDocument = QJsonDocument::fromJson(jsonData);
    if (jsonDocument.isNull()) {
        emit errorOccurred(tr("Failed to parse JSON data from file: %1").arg(fileName));
        return;
    }

    if (!jsonDocument.isArray()) {
        emit errorOccurred(tr("The JSON data is not an array in file: %1").arg(fileName));
        return;
    }

    auto jsonArray = jsonDocument.array();
    QMap<QString, QString> sites;
    QStringList ips;

    for (auto jsonValue : jsonArray) {
        auto jsonObject = jsonValue.toObject();
        auto hostname = jsonObject.value("hostname").toString("");
        auto ip = jsonObject.value("ip").toString("");

        if (!hostname.contains(".") && !NetworkUtilities::ipAddressWithSubnetRegExp().exactMatch(hostname)) {
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

    QMetaObject::invokeMethod(m_vpnConnection.get(), "addRoutes", Qt::QueuedConnection, Q_ARG(QStringList, ips));
    QMetaObject::invokeMethod(m_vpnConnection.get(), "flushDns", Qt::QueuedConnection);

    emit finished(tr("Import completed"));
}

void SitesController::exportSites(const QString &fileName)
{
    auto sites = m_sitesModel->getCurrentSites();

    QJsonArray jsonArray;

    for (const auto &site : sites) {
        QJsonObject jsonObject { { "hostname", site.first }, { "ip", site.second } };
        jsonArray.append(jsonObject);
    }

    QJsonDocument jsonDocument(jsonArray);
    QByteArray jsonData = jsonDocument.toJson();

    SystemController::saveFile(fileName, jsonData);

    emit finished(tr("Export completed"));
}
