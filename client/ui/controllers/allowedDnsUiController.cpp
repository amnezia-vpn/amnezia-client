#include "allowedDnsUiController.h"

#include <QDebug>
#include <QFile>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

#include "systemController.h"
#include "core/utils/networkUtilities.h"
#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"

AllowedDnsUiController::AllowedDnsUiController(AllowedDnsController* allowedDnsController,
                                                 AllowedDnsModel* allowedDnsModel,
                                                 QObject *parent)
    : QObject(parent),
      m_allowedDnsController(allowedDnsController),
      m_allowedDnsModel(allowedDnsModel)
{
    m_allowedDnsModel->updateModel(m_allowedDnsController->getCurrentDnsServers());
}

void AllowedDnsUiController::addDns(QString ip)
{
    if (ip.isEmpty()) {
        return;
    }

    if (!NetworkUtilities::ipAddressRegExp().match(ip).hasMatch()) {
        emit errorOccurred(tr("The address does not look like a valid IP address"));
        return;
    }

    if (m_allowedDnsController->addDns(ip)) {
        emit finished(tr("New DNS server added: %1").arg(ip));
    } else {
        emit errorOccurred(tr("DNS server already exists: %1").arg(ip));
    }
}

void AllowedDnsUiController::removeDns(int index)
{
    auto modelIndex = m_allowedDnsModel->index(index);
    auto ip = m_allowedDnsModel->data(modelIndex, AllowedDnsModel::Roles::IpRole).toString();
    m_allowedDnsController->removeDns(index);
    emit finished(tr("DNS server removed: %1").arg(ip));
}

void AllowedDnsUiController::importDns(const QString &fileName, bool replaceExisting)
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
    QStringList dnsServers;

    for (auto jsonValue : jsonArray) {
        auto ip = jsonValue.toString();
        
        if (!NetworkUtilities::ipAddressRegExp().match(ip).hasMatch()) {
            qDebug() << ip << " is not a valid IP address";
            continue;
        }
        
        dnsServers.append(ip);
    }

    m_allowedDnsController->addDnsList(dnsServers, replaceExisting);
    emit finished(tr("Import completed"));
}

void AllowedDnsUiController::exportDns(const QString &fileName)
{
    auto dnsServers = m_allowedDnsController->getCurrentDnsServers();

    QJsonArray jsonArray;

    for (const auto &ip : dnsServers) {
        jsonArray.append(ip);
    }

    QJsonDocument jsonDocument(jsonArray);
    QByteArray jsonData = jsonDocument.toJson();

    if (!SystemController::saveFile(fileName, jsonData)) {
        qInfo() << "AllowedDnsUiController::exportDns: save or share was cancelled or failed";
        return;
    }

    emit finished(tr("Export completed"));
}

void AllowedDnsUiController::updateModel()
{
    m_allowedDnsModel->updateModel(m_allowedDnsController->getCurrentDnsServers());
} 
