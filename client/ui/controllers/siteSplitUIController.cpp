#include "siteSplitUIController.h"

#include <QFile>
#include <QHostInfo>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

#include "systemController.h"
#include "core/networkUtilities.h"

SiteSplitUIController::SiteSplitUIController(QSharedPointer<SplitTunnelingController> splitTunnelingController,
                                             const QSharedPointer<SitesModel> &sitesModel, 
                                             QObject *parent)
    : QObject(parent), m_splitTunnelingController(splitTunnelingController), m_sitesModel(sitesModel)
{
}

void SiteSplitUIController::addSite(QString hostname)
{
    if (hostname.isEmpty()) {
        return;
    }

    if (!hostname.contains(".")) {
        emit errorOccurred(tr("Hostname not look like ip adress or domain name"));
        return;
    }

    if (m_splitTunnelingController->addSite(hostname)) {
        emit finished(tr("New site added: %1").arg(hostname));
    } else {
        emit errorOccurred(tr("Site already exists: %1").arg(hostname));
    }
}

void SiteSplitUIController::removeSite(int index)
{
    auto modelIndex = m_sitesModel->index(index);
    auto hostname = m_sitesModel->data(modelIndex, SitesModel::Roles::UrlRole).toString();
    
    if (m_splitTunnelingController->removeSite(hostname)) {
        emit finished(tr("Site removed: %1").arg(hostname));
    } else {
        emit errorOccurred(tr("Failed to remove site: %1").arg(hostname));
    }
}

void SiteSplitUIController::importSites(const QString &fileName, bool replaceExisting)
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

    for (auto jsonValue : jsonArray) {
        auto jsonObject = jsonValue.toObject();
        auto hostname = jsonObject.value("hostname").toString("");
        auto ip = jsonObject.value("ip").toString("");

        if (!hostname.contains(".") && !NetworkUtilities::ipAddressWithSubnetRegExp().exactMatch(hostname)) {
            qDebug() << hostname << " not look like ip adress or domain name";
            continue;
        }

        sites.insert(hostname, ip);
    }

    if (m_splitTunnelingController->addSites(sites, replaceExisting)) {
        emit finished(tr("Import completed"));
    } else {
        emit errorOccurred(tr("Import failed"));
    }
}

void SiteSplitUIController::exportSites(const QString &fileName)
{
    auto sites = m_splitTunnelingController->getSites(m_splitTunnelingController->getSitesRouteMode());

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
