#include "clientManagementController.h"

#include <QJsonDocument>
#include <QJsonObject>

#include "core/controllers/selfhosted/serverController.h"
#include "core/models/protocols/openvpnProtocolConfig.h"
#include "core/models/protocols/wireguardProtocolConfig.h"
#include "core/models/protocols/awgProtocolConfig.h"
#include "core/models/protocols/xrayProtocolConfig.h"
#include "core/models/protocols/shadowsocksProtocolConfig.h"
#include "core/models/protocols/cloakProtocolConfig.h"
#include "core/models/protocols/ikev2ProtocolConfig.h"
#include "core/models/protocols/protocolConfig.h"
#include "core/models/clientInfo.h"
#include <variant>
#include "settings.h"
#include "logger.h"
#include "protocols/protocols_defs.h"

using namespace amnezia;
using namespace amnezia::config_key;

namespace
{
    Logger logger("ClientManagementController");

    namespace configKey
    {
        constexpr char clientId[] = "clientId";
        constexpr char clientName[] = "clientName";
        constexpr char container[] = "container";
        constexpr char userData[] = "userData";
        constexpr char creationDate[] = "creationDate";
        constexpr char latestHandshake[] = "latestHandshake";
        constexpr char dataReceived[] = "dataReceived";
        constexpr char dataSent[] = "dataSent";
        constexpr char allowedIps[] = "allowedIps";
    }
}

ClientManagementController::ClientManagementController(std::shared_ptr<Settings> settings, QObject *parent)
    : QObject(parent), m_settings(settings)
{
}

QString ClientManagementController::getClientsTableFilePath(const DockerContainer container)
{
    QString clientsTableFile = QString("/opt/amnezia/%1/clientsTable");
    if (container == DockerContainer::OpenVpn || container == DockerContainer::ShadowSocks || container == DockerContainer::Cloak) {
        clientsTableFile = clientsTableFile.arg(ContainerProps::containerTypeToString(DockerContainer::OpenVpn));
    } else {
        clientsTableFile = clientsTableFile.arg(ContainerProps::containerTypeToString(container));
    }
    return clientsTableFile;
}

void ClientManagementController::migration(const QByteArray &clientsTableString, QList<ClientInfo> &clientsList)
{
    QJsonObject clientsTableObj = QJsonDocument::fromJson(clientsTableString).object();

    for (auto &clientId : clientsTableObj.keys()) {
        ClientInfo client;
        client.clientId = clientId;
        client.clientName = clientsTableObj.value(clientId).toObject().value(configKey::clientName).toString();
        client.creationDate = QDateTime::currentDateTime();

        clientsList.append(client);
    }
}




ErrorCode ClientManagementController::updateClientsData(const DockerContainer container, const ServerCredentials &credentials,
                                                        const QSharedPointer<ServerController> &serverController, QList<ClientInfo> &clientsList)
{
    clientsList.clear();
    ErrorCode error = ErrorCode::NoError;

    QString clientsTableFile = getClientsTableFilePath(container);
    const QByteArray clientsTableString = serverController->getTextFileFromContainer(container, credentials, clientsTableFile, error);
    if (error != ErrorCode::NoError) {
        logger.error() << "Failed to get the clientsTable file from the server";
        return error;
    }

    QJsonArray clientsTable = QJsonDocument::fromJson(clientsTableString).array();

    if (clientsTable.isEmpty()) {
        migration(clientsTableString, clientsList);
        const QByteArray newClientsTableString = QJsonDocument(clientsToJsonArray(clientsList)).toJson();
        error = serverController->uploadTextFileToContainer(container, credentials, newClientsTableString, clientsTableFile);
        if (error != ErrorCode::NoError) {
            logger.error() << "Failed to upload the clientsTable file to the server";
            return error;
        }
    } else {
        clientsList = clientsFromJsonArray(clientsTable);
    }

    int count = 0;
    switch (container) {
    case DockerContainer::OpenVpn:
    case DockerContainer::ShadowSocks:
    case DockerContainer::Cloak:
        error = getOpenVpnClients(container, credentials, serverController, count, clientsList);
        break;
    case DockerContainer::WireGuard:
    case DockerContainer::Awg:
        error = getWireGuardClients(container, credentials, serverController, count, clientsList);
        break;
    case DockerContainer::Xray:
        error = getXrayClients(container, credentials, serverController, count, clientsList);
        break;
    default:
        error = ErrorCode::NoError;
        break;
    }

    if (error != ErrorCode::NoError) {
        logger.error() << "Failed to get clients for container" << ContainerProps::containerTypeToString(container);
        return error;
    }

    emit clientsDataUpdated(clientsList);
    return ErrorCode::NoError;
}

// UI-facing methods - create ServerController internally
ErrorCode ClientManagementController::updateClientsData(const DockerContainer container, const ServerCredentials &credentials)
{
    QSharedPointer<ServerController> serverController(new ServerController(m_settings));
    QList<ClientInfo> clientsList;
    return updateClientsData(container, credentials, serverController, clientsList);
}





ErrorCode ClientManagementController::appendClient(const DockerContainer container, const ServerCredentials &credentials,
                                                   const ContainerConfig &containerConfig, const QString &clientName,
                                                   const QSharedPointer<ServerController> &serverController, QList<ClientInfo> &clientsList)
{
    Proto protocol;
    switch (container) {
        case DockerContainer::ShadowSocks:
        case DockerContainer::Cloak:
            protocol = Proto::OpenVpn;
            break;
        case DockerContainer::OpenVpn:
        case DockerContainer::WireGuard:
        case DockerContainer::Awg:
        case DockerContainer::Xray:
            protocol = ContainerProps::defaultProtocol(container);
            break;
        default:
            return ErrorCode::NoError;
    }

    QString protocolName = ProtocolProps::protoToString(protocol);
    auto protocolConfig = containerConfig.protocolConfigs.value(protocolName);
    return appendClient(protocolConfig, clientName, container, credentials, serverController, clientsList);
}



ErrorCode ClientManagementController::appendClient(QSharedPointer<ProtocolConfig> &protocolConfig, const QString &clientName, const DockerContainer container,
                                                   const ServerCredentials &credentials, const QSharedPointer<ServerController> &serverController,
                                                   QList<ClientInfo> &clientsList)
{
    QString clientId;
    if (container == DockerContainer::Xray) {
        if (!protocolConfig) {
            return ErrorCode::InternalError;
        }
        QJsonObject protocolConfigJson = protocolConfig->toJson();
        if (!protocolConfigJson.contains("outbounds")) {
            return ErrorCode::InternalError;
        }
        QJsonArray outbounds = protocolConfigJson.value("outbounds").toArray();
        if (outbounds.isEmpty()) {
            return ErrorCode::InternalError;
        }
        QJsonObject outbound = outbounds[0].toObject();
        if (!outbound.contains("settings")) {
            return ErrorCode::InternalError;
        }
        QJsonObject settings = outbound["settings"].toObject();
        if (!settings.contains("vnext")) {
            return ErrorCode::InternalError;
        }
        QJsonArray vnext = settings["vnext"].toArray();
        if (vnext.isEmpty()) {
            return ErrorCode::InternalError;
        }
        QJsonObject vnextObj = vnext[0].toObject();
        if (!vnextObj.contains("users")) {
            return ErrorCode::InternalError;
        }
        QJsonArray users = vnextObj["users"].toArray();
        if (users.isEmpty()) {
            return ErrorCode::InternalError;
        }
        QJsonObject user = users[0].toObject();
        if (!user.contains("id")) {
            return ErrorCode::InternalError;
        }
        clientId = user["id"].toString();
    } else {
        if (!protocolConfig) {
            return ErrorCode::InternalError;
        }
        ProtocolConfigVariant variant = ProtocolConfig::getProtocolConfigVariant(protocolConfig);
        std::visit([&clientId](const auto &config) -> void {
            clientId = config->clientProtocolConfig.clientId;
        }, variant);
    }
    
    return appendClient(clientId, clientName, container, credentials, serverController, clientsList);
}



ErrorCode ClientManagementController::appendClient(const QString &clientId, const QString &clientName, const DockerContainer container,
                                                   const ServerCredentials &credentials, const QSharedPointer<ServerController> &serverController,
                                                   QList<ClientInfo> &clientsList)
{
    ErrorCode error = ErrorCode::NoError;

    QList<ClientInfo> currentClients;
    error = updateClientsData(container, credentials, serverController, currentClients);
    if (error != ErrorCode::NoError) {
        return error;
    }

    for (int i = 0; i < currentClients.size(); ++i) {
        if (currentClients[i].clientId == clientId) {
            return renameClient(i, clientName, container, credentials, serverController, currentClients, true);
        }
    }

    ClientInfo newClient(clientId, clientName);
    newClient.container = container;
    currentClients.append(newClient);

    const QByteArray clientsTableString = QJsonDocument(clientsToJsonArray(currentClients)).toJson();
    QString clientsTableFile = getClientsTableFilePath(container);

    error = serverController->uploadTextFileToContainer(container, credentials, clientsTableString, clientsTableFile);
    if (error != ErrorCode::NoError) {
        logger.error() << "Failed to upload the clientsTable file to the server";
        return error;
    }

    clientsList = currentClients;
    emit clientAdded(clientId, clientName);
    return ErrorCode::NoError;
}

ErrorCode ClientManagementController::renameClient(const int row, const QString &clientName, const DockerContainer container, 
                                                   const ServerCredentials &credentials, bool addTimeStamp)
{
    QSharedPointer<ServerController> serverController(new ServerController(m_settings));
    QList<ClientInfo> clientsList;
    ErrorCode errorCode = updateClientsData(container, credentials, serverController, clientsList);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }
    return renameClient(row, clientName, container, credentials, serverController, clientsList, addTimeStamp);
}

ErrorCode ClientManagementController::renameClient(const int row, const QString &clientName, const DockerContainer container,
                                                   const ServerCredentials &credentials, const QSharedPointer<ServerController> &serverController,
                                                   QList<ClientInfo> &clientsList, bool addTimeStamp)
{
    if (row < 0 || row >= clientsList.size()) {
        return ErrorCode::InternalError;
    }

    clientsList[row].clientName = clientName;
    if (addTimeStamp) {
        clientsList[row].creationDate = QDateTime::currentDateTime();
    }

    const QByteArray clientsTableString = QJsonDocument(clientsToJsonArray(clientsList)).toJson();
    QString clientsTableFile = getClientsTableFilePath(container);

    ErrorCode error = serverController->uploadTextFileToContainer(container, credentials, clientsTableString, clientsTableFile);
    if (error != ErrorCode::NoError) {
        logger.error() << "Failed to upload the clientsTable file to the server";
        return error;
    }

    emit clientRenamed(row, clientName);
    return ErrorCode::NoError;
}

 
 
ErrorCode ClientManagementController::getWireGuardClients(const DockerContainer container, const ServerCredentials &credentials,
                                                          const QSharedPointer<ServerController> &serverController, int &count, QList<ClientInfo> &clientsList)
{
    ErrorCode error = ErrorCode::NoError;

    const QString wireGuardConfigFile = QString("opt/amnezia/%1/wg0.conf").arg(container == DockerContainer::WireGuard ? "wireguard" : "awg");
    const QString wireguardConfigString = serverController->getTextFileFromContainer(container, credentials, wireGuardConfigFile, error);
    if (error != ErrorCode::NoError) {
        logger.error() << "Failed to get the wg conf file from the server";
        return error;
    }

    auto configLines = wireguardConfigString.split("\n", Qt::SkipEmptyParts);
    QStringList wireguardKeys;
    for (const auto &line : configLines) {
        auto configPair = line.split(" = ", Qt::SkipEmptyParts);
        if (configPair.front() == "PublicKey") {
            wireguardKeys.push_back(configPair.back());
        }
    }

    for (auto &wireguardKey : wireguardKeys) {
        if (!isClientExists(wireguardKey, clientsList)) {
            ClientInfo client(wireguardKey, QString("Client %1").arg(count));
            client.container = container;
            clientsList.append(client);
            count++;
        }
    }
    return error;
}

ErrorCode ClientManagementController::getXrayClients(const DockerContainer container, const ServerCredentials& credentials,
                                                     const QSharedPointer<ServerController> &serverController, int &count, QList<ClientInfo> &clientsList)
{
    ErrorCode error = ErrorCode::NoError;

    const QString serverConfigPath = amnezia::protocols::xray::serverConfigPath;
    const QString configString = serverController->getTextFileFromContainer(container, credentials, serverConfigPath, error);
    if (error != ErrorCode::NoError) {
        logger.error() << "Failed to get the xray server config file from the server";
        return error;
    }

    QJsonDocument serverConfig = QJsonDocument::fromJson(configString.toUtf8());
    if (serverConfig.isNull()) {
        logger.error() << "Failed to parse xray server config JSON";
        return ErrorCode::InternalError;
    }

    if (!serverConfig.object().contains("inbounds") || serverConfig.object()["inbounds"].toArray().isEmpty()) {
        logger.error() << "Invalid xray server config structure";
        return ErrorCode::InternalError;
    }

    const QJsonObject inbound = serverConfig.object()["inbounds"].toArray()[0].toObject();
    if (!inbound.contains("settings")) {
        logger.error() << "Missing settings in xray inbound config";
        return ErrorCode::InternalError;
    }

    const QJsonObject settings = inbound["settings"].toObject();
    if (!settings.contains("clients")) {
        logger.error() << "Missing clients in xray settings config"; 
        return ErrorCode::InternalError;
    }

    const QJsonArray clients = settings["clients"].toArray();
    for (const auto &clientValue : clients) {
        const QJsonObject clientObj = clientValue.toObject();
        if (!clientObj.contains("id")) {
            logger.error() << "Missing id in xray client config";
            continue;
        }
        QString clientId = clientObj["id"].toString();
        
        QString xrayDefaultUuid = serverController->getTextFileFromContainer(container, credentials, amnezia::protocols::xray::uuidPath, error);
        xrayDefaultUuid.replace("\n", "");

        if (!isClientExists(clientId, clientsList) && clientId != xrayDefaultUuid) {
            ClientInfo client(clientId, QString("Client %1").arg(count));
            client.container = container;
            clientsList.append(client);
            count++;
        }
    }
    
    return error;
}

ErrorCode ClientManagementController::getOpenVpnClients(const DockerContainer container, const ServerCredentials &credentials,
                                                       const QSharedPointer<ServerController> &serverController, int &count, QList<ClientInfo> &clientsList)
{
    Q_UNUSED(container);
    Q_UNUSED(credentials);
    Q_UNUSED(serverController);
    count = clientsList.size();
    return ErrorCode::NoError;
}

ErrorCode ClientManagementController::wgShow(const DockerContainer container, const ServerCredentials &credentials,
                                            const QSharedPointer<ServerController> &serverController, std::vector<WgShowData> &data)
{
    if (container != DockerContainer::WireGuard && container != DockerContainer::Awg) {
        return ErrorCode::NoError;
    }

    ErrorCode error = ErrorCode::NoError;
    QString stdOut;
    auto cbReadStdOut = [&](const QString &data, libssh::Client &) {
        stdOut += data + "\n";
        return ErrorCode::NoError;
    };

    const QString command = QString("sudo docker exec -i $CONTAINER_NAME bash -c '%1'").arg("wg show all");

    QString script = serverController->replaceVars(command, serverController->generateVarsForContainer(credentials, container));
    error = serverController->runScript(credentials, script, cbReadStdOut);
    if (error != ErrorCode::NoError) {
        logger.error() << "Failed to execute wg show command";
        return error;
    }

    if (stdOut.isEmpty()) {
        return error;
    }

    const auto getStrValue = [](const auto str) { return str.mid(str.indexOf(":") + 1).trimmed(); };

    const auto parts = stdOut.split('\n');
    const auto peerList = parts.filter("peer:");
    const auto latestHandshakeList = parts.filter("latest handshake:");
    const auto transferredDataList = parts.filter("transfer:");
    const auto allowedIpsList = parts.filter("allowed ips:");

    if (allowedIpsList.isEmpty() || latestHandshakeList.isEmpty() || transferredDataList.isEmpty() || peerList.isEmpty()) {
        return error;
    }

    const auto changeHandshakeFormat = [](QString &latestHandshake) {
        const std::vector<std::pair<QString, QString>> replaceMap = { { " days", "d" },    { " hours", "h" }, { " minutes", "m" },
                                                                      { " seconds", "s" }, { " day", "d" },   { " hour", "h" },
                                                                      { " minute", "m" },  { " second", "s" } };

        for (const auto &item : replaceMap) {
            latestHandshake.replace(item.first, item.second);
        }
    };

    for (int i = 0; i < peerList.size() && i < transferredDataList.size() && i < latestHandshakeList.size() && i < allowedIpsList.size(); ++i) {

        const auto transferredData = getStrValue(transferredDataList[i]).split(",");
        auto latestHandshake = getStrValue(latestHandshakeList[i]);
        auto serverBytesReceived = transferredData.front().trimmed();
        auto serverBytesSent = transferredData.back().trimmed();
        auto allowedIps = getStrValue(allowedIpsList[i]);

        changeHandshakeFormat(latestHandshake);

        serverBytesReceived.chop(QStringLiteral(" received").length());
        serverBytesSent.chop(QStringLiteral(" sent").length());

        data.push_back({ getStrValue(peerList[i]), latestHandshake, serverBytesSent, serverBytesReceived, allowedIps });
    }

    return error;
}



ErrorCode ClientManagementController::revokeOpenVpn(const int row, const DockerContainer container, const ServerCredentials &credentials,
                                                   const int serverIndex, const QSharedPointer<ServerController> &serverController)
{
    QList<ClientInfo> clientsList;
    ErrorCode errorCode = updateClientsData(container, credentials, serverController, clientsList);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }
    return revokeOpenVpn(row, container, credentials, serverIndex, serverController, clientsList);
}

ErrorCode ClientManagementController::revokeWireGuard(const int row, const DockerContainer container, const ServerCredentials &credentials,
                                                     const QSharedPointer<ServerController> &serverController)
{
    QList<ClientInfo> clientsList;
    ErrorCode errorCode = updateClientsData(container, credentials, serverController, clientsList);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }
    return revokeWireGuard(row, container, credentials, serverController, clientsList);
}

ErrorCode ClientManagementController::revokeXray(const int row, const DockerContainer container, const ServerCredentials &credentials,
                                                const QSharedPointer<ServerController> &serverController)
{
    QList<ClientInfo> clientsList;
    ErrorCode errorCode = updateClientsData(container, credentials, serverController, clientsList);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }
    return revokeXray(row, container, credentials, serverController, clientsList);
} 


ErrorCode ClientManagementController::revokeClient(const int row, const DockerContainer container,
                                                   const ServerCredentials &credentials, const int serverIndex)
{
    QSharedPointer<ServerController> serverController(new ServerController(m_settings));
    QList<ClientInfo> clientsList;
    ErrorCode errorCode = updateClientsData(container, credentials, serverController, clientsList);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }
    return revokeClient(row, container, credentials, serverIndex, serverController, clientsList);
}


ErrorCode ClientManagementController::revokeClient(const int row, const DockerContainer container, const ServerCredentials &credentials,
                                                   const int serverIndex, const QSharedPointer<ServerController> &serverController,
                                                   QList<ClientInfo> &clientsList)
{
    if (row < 0 || row >= clientsList.size()) {
        return ErrorCode::InternalError;
    }

    QString clientId = clientsList[row].clientId;
    
    ErrorCode errorCode = ErrorCode::NoError;

    switch(container) {
        case DockerContainer::OpenVpn:
        case DockerContainer::ShadowSocks:
        case DockerContainer::Cloak: {
            errorCode = revokeOpenVpn(row, container, credentials, serverIndex, serverController, clientsList);
            break;
        }
        case DockerContainer::WireGuard:
        case DockerContainer::Awg: {
            errorCode = revokeWireGuard(row, container, credentials, serverController, clientsList);
            break;
        }
        case DockerContainer::Xray: {
            errorCode = revokeXray(row, container, credentials, serverController, clientsList);
            break;
        }
        default: {
            logger.error() << "Internal error: received unexpected container type";
            return ErrorCode::InternalError;
        }
    }

    if (errorCode == ErrorCode::NoError) {
        emit clientRevoked(row);
    }

    return errorCode;
}

ErrorCode ClientManagementController::revokeClient(const ContainerConfig &containerConfig, const DockerContainer container,
                                                   const ServerCredentials &credentials, const int serverIndex,
                                                   const QSharedPointer<ServerController> &serverController, QList<ClientInfo> &clientsList)
{

    Proto protocol;
    switch(container) {
        case DockerContainer::ShadowSocks:
        case DockerContainer::Cloak: {
            protocol = Proto::OpenVpn;
            break;
        }
        case DockerContainer::OpenVpn:
        case DockerContainer::WireGuard:
        case DockerContainer::Awg:
        case DockerContainer::Xray: {
            protocol = ContainerProps::defaultProtocol(container);
            break;
        }
        default: {
            logger.error() << "Internal error: received unexpected container type";
            return ErrorCode::InternalError;
        }
    }

    auto protocolConfig = ContainerProps::getProtocolConfigFromContainer(protocol, containerConfig);


    QString clientId;
    if (container == DockerContainer::Xray) {
        if (!protocolConfig) {
            return ErrorCode::InternalError;
        }
        QJsonObject protocolConfigJson = protocolConfig->toJson();
        if (!protocolConfigJson.contains("outbounds")) {
            return ErrorCode::InternalError;
        }
        QJsonArray outbounds = protocolConfigJson.value("outbounds").toArray();
        if (outbounds.isEmpty()) {
            return ErrorCode::InternalError;
        }
        QJsonObject outbound = outbounds[0].toObject();
        if (!outbound.contains("settings")) {
            return ErrorCode::InternalError;
        }
        QJsonObject settings = outbound["settings"].toObject();
        if (!settings.contains("vnext")) {
            return ErrorCode::InternalError;
        }
        QJsonArray vnext = settings["vnext"].toArray();
        if (vnext.isEmpty()) {
            return ErrorCode::InternalError;
        }
        QJsonObject vnextObj = vnext[0].toObject();
        if (!vnextObj.contains("users")) {
            return ErrorCode::InternalError;
        }
        QJsonArray users = vnextObj["users"].toArray();
        if (users.isEmpty()) {
            return ErrorCode::InternalError;
        }
        QJsonObject user = users[0].toObject();
        if (!user.contains("id")) {
            return ErrorCode::InternalError;
        }
        clientId = user["id"].toString();
    } else {
        if (!protocolConfig) {
            return ErrorCode::InternalError;
        }
        ProtocolConfigVariant variant = ProtocolConfig::getProtocolConfigVariant(protocolConfig);
        std::visit([&clientId](const auto &config) -> void {
            clientId = config->clientProtocolConfig.clientId;
        }, variant);
    }


    for (int i = 0; i < clientsList.size(); i++) {
        if (clientsList[i].clientId == clientId) {
            return revokeClient(i, container, credentials, serverIndex, serverController, clientsList);
        }
    }

    return ErrorCode::InternalError;
}

ErrorCode ClientManagementController::revokeOpenVpn(const int row, const DockerContainer container, const ServerCredentials &credentials,
                                                   const int serverIndex, const QSharedPointer<ServerController> &serverController,
                                                   QList<ClientInfo> &clientsList)
{
    if (row < 0 || row >= clientsList.size()) {
        return ErrorCode::InternalError;
    }
    QString clientId = clientsList[row].clientId;
    
    ErrorCode error = ErrorCode::NoError;
    QString stdOut;
    auto cbReadStdOut = [&](const QString &data, libssh::Client &) {
        stdOut += data + "\n";
    };

    const QString revokeClientScript = "sudo docker exec -i $CONTAINER_NAME bash -c './easyrsa --batch revoke %1'";
    QString script = serverController->replaceVars(revokeClientScript.arg(clientId), 
                                                   serverController->generateVarsForContainer(credentials, container));
    error = serverController->runScript(credentials, script, cbReadStdOut);
    if (error != ErrorCode::NoError) {
        return error;
    }

    clientsList.removeAt(row);
    const QByteArray clientsTableString = QJsonDocument(clientsToJsonArray(clientsList)).toJson();
    QString clientsTableFile = getClientsTableFilePath(container);
    
    error = serverController->uploadTextFileToContainer(container, credentials, clientsTableString, clientsTableFile);
    if (error != ErrorCode::NoError) {
        logger.error() << "Failed to upload the clientsTable file to the server";
        return error;
    }

    return ErrorCode::NoError;
}

ErrorCode ClientManagementController::revokeWireGuard(const int row, const DockerContainer container, const ServerCredentials &credentials,
                                                     const QSharedPointer<ServerController> &serverController, QList<ClientInfo> &clientsList)
{
    if (row < 0 || row >= clientsList.size()) {
        return ErrorCode::InternalError;
    }
    QString clientId = clientsList[row].clientId;
    
    ErrorCode error = ErrorCode::NoError;
    
    const QString wireGuardConfigFile = QString("opt/amnezia/%1/wg0.conf").arg(container == DockerContainer::WireGuard ? "wireguard" : "awg");
    const QString wireguardConfigString = serverController->getTextFileFromContainer(container, credentials, wireGuardConfigFile, error);
    if (error != ErrorCode::NoError) {
        return error;
    }

    QStringList configLines = wireguardConfigString.split("\n");
    QStringList newConfigLines;
    bool skipPeer = false;
    
    for (const QString &line : configLines) {
        if (line.startsWith("[Peer]")) {
            skipPeer = false;
        }
        if (line.contains("PublicKey") && line.contains(clientId)) {
            skipPeer = true;
            continue;
        }
        if (!skipPeer) {
            newConfigLines.append(line);
        }
    }

    QString newConfig = newConfigLines.join("\n");
    error = serverController->uploadTextFileToContainer(container, credentials, newConfig.toUtf8(), wireGuardConfigFile);
    if (error != ErrorCode::NoError) {
        return error;
    }

    clientsList.removeAt(row);
    const QByteArray clientsTableString = QJsonDocument(clientsToJsonArray(clientsList)).toJson();
    QString clientsTableFile = getClientsTableFilePath(container);
    
    error = serverController->uploadTextFileToContainer(container, credentials, clientsTableString, clientsTableFile);
    if (error != ErrorCode::NoError) {
        logger.error() << "Failed to upload the clientsTable file to the server";
        return error;
    }

    return ErrorCode::NoError;
}

ErrorCode ClientManagementController::revokeXray(const int row, const DockerContainer container, const ServerCredentials &credentials,
                                                const QSharedPointer<ServerController> &serverController, QList<ClientInfo> &clientsList)
{
    if (row < 0 || row >= clientsList.size()) {
        return ErrorCode::InternalError;
    }
    QString clientId = clientsList[row].clientId;
    
    ErrorCode error = ErrorCode::NoError;
    
    const QString serverConfigPath = amnezia::protocols::xray::serverConfigPath;
    const QString configString = serverController->getTextFileFromContainer(container, credentials, serverConfigPath, error);
    if (error != ErrorCode::NoError) {
        return error;
    }

    QJsonDocument serverConfig = QJsonDocument::fromJson(configString.toUtf8());
    if (serverConfig.isNull()) {
        return ErrorCode::InternalError;
    }

    QJsonObject configObj = serverConfig.object();
    QJsonArray inbounds = configObj["inbounds"].toArray();
    
    for (int i = 0; i < inbounds.size(); i++) {
        QJsonObject inbound = inbounds[i].toObject();
        QJsonObject settings = inbound["settings"].toObject();
        QJsonArray clients = settings["clients"].toArray();
        
        for (int j = 0; j < clients.size(); j++) {
            QJsonObject clientObj = clients[j].toObject();
            if (clientObj["id"].toString() == clientId) {
                clients.removeAt(j);
                settings["clients"] = clients;
                inbound["settings"] = settings;
                inbounds[i] = inbound;
                break;
            }
        }
    }
    
    configObj["inbounds"] = inbounds;
    QJsonDocument newServerConfig(configObj);
    
    error = serverController->uploadTextFileToContainer(container, credentials, newServerConfig.toJson(), serverConfigPath);
    if (error != ErrorCode::NoError) {
        return error;
    }

    clientsList.removeAt(row);
    const QByteArray clientsTableString = QJsonDocument(clientsToJsonArray(clientsList)).toJson();
    QString clientsTableFile = getClientsTableFilePath(container);
    
    error = serverController->uploadTextFileToContainer(container, credentials, clientsTableString, clientsTableFile);
    if (error != ErrorCode::NoError) {
        logger.error() << "Failed to upload the clientsTable file to the server";
        return error;
    }

    return ErrorCode::NoError;
}

QList<ClientInfo> ClientManagementController::clientsFromJsonArray(const QJsonArray &jsonArray)
{
    QList<ClientInfo> clientsList;
    for (const auto &value : jsonArray) {
        clientsList.append(ClientInfo(value.toObject()));
    }
    return clientsList;
}

QJsonArray ClientManagementController::clientsToJsonArray(const QList<ClientInfo> &clientsList)
{
    QJsonArray jsonArray;
    for (const auto &client : clientsList) {
        jsonArray.append(client.toJson());
    }
    return jsonArray;
}

bool ClientManagementController::isClientExists(const QString &clientId, const QList<ClientInfo> &clientsList)
{
    for (const auto &client : clientsList) {
        if (client.clientId == clientId) {
            return true;
        }
    }
    return false;
} 
