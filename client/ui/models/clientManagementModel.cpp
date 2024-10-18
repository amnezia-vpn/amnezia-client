#include "clientManagementModel.h"

#include <QJsonDocument>
#include <QJsonObject>

#include "core/controllers/serverController.h"
#include "logger.h"

namespace
{
    Logger logger("ClientManagementModel");

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

ClientManagementModel::ClientManagementModel(std::shared_ptr<Settings> settings, QObject *parent)
    : m_settings(settings), QAbstractListModel(parent)
{
}

int ClientManagementModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return static_cast<int>(m_clientsTable.size());
}

QVariant ClientManagementModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_clientsTable.size())) {
        return QVariant();
    }

    auto client = m_clientsTable.at(index.row()).toObject();
    auto userData = client.value(configKey::userData).toObject();

    switch (role) {
    case ClientNameRole: return userData.value(configKey::clientName).toString();
    case CreationDateRole: return userData.value(configKey::creationDate).toString();
    case LatestHandshakeRole: return userData.value(configKey::latestHandshake).toString();
    case DataReceivedRole: return userData.value(configKey::dataReceived).toString();
    case DataSentRole: return userData.value(configKey::dataSent).toString();
    case AllowedIpsRole: return userData.value(configKey::allowedIps).toString();
    }

    return QVariant();
}

void ClientManagementModel::migration(const QByteArray &clientsTableString)
{
    QJsonObject clientsTable = QJsonDocument::fromJson(clientsTableString).object();

    for (auto &clientId : clientsTable.keys()) {
        QJsonObject client;
        client[configKey::clientId] = clientId;

        QJsonObject userData;
        userData[configKey::clientName] = clientsTable.value(clientId).toObject().value(configKey::clientName);
        client[configKey::userData] = userData;

        m_clientsTable.push_back(client);
    }
}

ErrorCode ClientManagementModel::updateModel(const DockerContainer container, const ServerCredentials &credentials,
                                             const QSharedPointer<ServerController> &serverController)
{
    beginResetModel();
    m_clientsTable = QJsonArray();
    endResetModel();

    ErrorCode error = ErrorCode::NoError;

    QString clientsTableFile = QString("/opt/amnezia/%1/clientsTable");
    if (container == DockerContainer::OpenVpn || container == DockerContainer::ShadowSocks || container == DockerContainer::Cloak) {
        clientsTableFile = clientsTableFile.arg(ContainerProps::containerTypeToString(DockerContainer::OpenVpn));
    } else {
        clientsTableFile = clientsTableFile.arg(ContainerProps::containerTypeToString(container));
    }

    const QByteArray clientsTableString = serverController->getTextFileFromContainer(container, credentials, clientsTableFile, error);
    if (error != ErrorCode::NoError) {
        logger.error() << "Failed to get the clientsTable file from the server";
        return error;
    }

    beginResetModel();
    m_clientsTable = QJsonDocument::fromJson(clientsTableString).array();

    if (m_clientsTable.isEmpty()) {
        migration(clientsTableString);

        int count = 0;

        if (container == DockerContainer::OpenVpn || container == DockerContainer::ShadowSocks || container == DockerContainer::Cloak) {
            error = getOpenVpnClients(container, credentials, serverController, count);
        } else if (container == DockerContainer::WireGuard || container == DockerContainer::Awg) {
            error = getWireGuardClients(container, credentials, serverController, count);
        }
        if (error != ErrorCode::NoError) {
            endResetModel();
            return error;
        }

        const QByteArray newClientsTableString = QJsonDocument(m_clientsTable).toJson();
        if (clientsTableString != newClientsTableString) {
            error = serverController->uploadTextFileToContainer(container, credentials, newClientsTableString, clientsTableFile);
            if (error != ErrorCode::NoError) {
                logger.error() << "Failed to upload the clientsTable file to the server";
            }
        }
    }

    std::vector<WgShowData> data;
    wgShow(container, credentials, serverController, data);

    for (const auto &client : data) {
        int i = 0;
        for (const auto &it : std::as_const(m_clientsTable)) {
            if (it.isObject()) {
                QJsonObject obj = it.toObject();
                if (obj.contains(configKey::clientId) && obj[configKey::clientId].toString() == client.clientId) {
                    QJsonObject userData = obj[configKey::userData].toObject();

                    if (!client.latestHandshake.isEmpty()) {
                        userData[configKey::latestHandshake] = client.latestHandshake;
                    }

                    if (!client.dataReceived.isEmpty()) {
                        userData[configKey::dataReceived] = client.dataReceived;
                    }

                    if (!client.dataSent.isEmpty()) {
                        userData[configKey::dataSent] = client.dataSent;
                    }

                    if (!client.allowedIps.isEmpty()) {
                        userData[configKey::allowedIps] = client.allowedIps;
                    }

                    obj[configKey::userData] = userData;
                    m_clientsTable.replace(i, obj);
                    break;
                }
            }
            ++i;
        }
    }

    endResetModel();
    return error;
}

ErrorCode ClientManagementModel::getOpenVpnClients(const DockerContainer container, const ServerCredentials &credentials,
                                                   const QSharedPointer<ServerController> &serverController, int &count)
{
    ErrorCode error = ErrorCode::NoError;
    QString stdOut;
    auto cbReadStdOut = [&](const QString &data, libssh::Client &) {
        stdOut += data + "\n";
        return ErrorCode::NoError;
    };

    const QString getOpenVpnClientsList = "sudo docker exec -i $CONTAINER_NAME bash -c 'ls /opt/amnezia/openvpn/pki/issued'";
    QString script = serverController->replaceVars(getOpenVpnClientsList, serverController->genVarsForScript(credentials, container));
    error = serverController->runScript(credentials, script, cbReadStdOut);
    if (error != ErrorCode::NoError) {
        logger.error() << "Failed to retrieve the list of issued certificates on the server";
        return error;
    }

    if (!stdOut.isEmpty()) {
        QStringList certsIds = stdOut.split("\n", Qt::SkipEmptyParts);
        certsIds.removeAll("AmneziaReq.crt");

        for (auto &openvpnCertId : certsIds) {
            openvpnCertId.replace(".crt", "");
            if (!isClientExists(openvpnCertId)) {
                QJsonObject client;
                client[configKey::clientId] = openvpnCertId;

                QJsonObject userData;
                userData[configKey::clientName] = QString("Client %1").arg(count);
                client[configKey::userData] = userData;

                m_clientsTable.push_back(client);

                count++;
            }
        }
    }
    return error;
}

ErrorCode ClientManagementModel::getWireGuardClients(const DockerContainer container, const ServerCredentials &credentials,
                                                     const QSharedPointer<ServerController> &serverController, int &count)
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
        if (!isClientExists(wireguardKey)) {
            QJsonObject client;
            client[configKey::clientId] = wireguardKey;

            QJsonObject userData;
            userData[configKey::clientName] = QString("Client %1").arg(count);
            client[configKey::userData] = userData;

            m_clientsTable.push_back(client);

            count++;
        }
    }
    return error;
}

ErrorCode ClientManagementModel::wgShow(const DockerContainer container, const ServerCredentials &credentials,
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

    QString script = serverController->replaceVars(command, serverController->genVarsForScript(credentials, container));
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

bool ClientManagementModel::isClientExists(const QString &clientId)
{
    for (const QJsonValue &value : std::as_const(m_clientsTable)) {
        if (value.isObject()) {
            QJsonObject obj = value.toObject();
            if (obj.contains(configKey::clientId) && obj[configKey::clientId].toString() == clientId) {
                return true;
            }
        }
    }
    return false;
}

ErrorCode ClientManagementModel::appendClient(const DockerContainer container, const ServerCredentials &credentials,
                                              const QJsonObject &containerConfig, const QString &clientName,
                                              const QSharedPointer<ServerController> &serverController)
{
    Proto protocol;
    if (container == DockerContainer::ShadowSocks || container == DockerContainer::Cloak) {
        protocol = Proto::OpenVpn;
    } else if (container == DockerContainer::OpenVpn || container == DockerContainer::WireGuard || container == DockerContainer::Awg) {
        protocol = ContainerProps::defaultProtocol(container);
    } else {
        return ErrorCode::NoError;
    }

    auto protocolConfig = ContainerProps::getProtocolConfigFromContainer(protocol, containerConfig);

    return appendClient(protocolConfig.value(config_key::clientId).toString(), clientName, container, credentials, serverController);
}

ErrorCode ClientManagementModel::appendClient(const QString &clientId, const QString &clientName, const DockerContainer container,
                                              const ServerCredentials &credentials, const QSharedPointer<ServerController> &serverController)
{
    ErrorCode error = ErrorCode::NoError;

    error = updateModel(container, credentials, serverController);
    if (error != ErrorCode::NoError) {
        return error;
    }

    for (int i = 0; i < m_clientsTable.size(); i++) {
        if (m_clientsTable.at(i).toObject().value(configKey::clientId) == clientId) {
            return renameClient(i, clientName, container, credentials, serverController, true);
        }
    }

    beginInsertRows(QModelIndex(), rowCount(), rowCount() + 1);
    QJsonObject client;
    client[configKey::clientId] = clientId;

    QJsonObject userData;
    userData[configKey::clientName] = clientName;
    userData[configKey::creationDate] = QDateTime::currentDateTime().toString();
    client[configKey::userData] = userData;
    m_clientsTable.push_back(client);
    endInsertRows();

    const QByteArray clientsTableString = QJsonDocument(m_clientsTable).toJson();

    QString clientsTableFile = QString("/opt/amnezia/%1/clientsTable");
    if (container == DockerContainer::OpenVpn || container == DockerContainer::ShadowSocks || container == DockerContainer::Cloak) {
        clientsTableFile = clientsTableFile.arg(ContainerProps::containerTypeToString(DockerContainer::OpenVpn));
    } else {
        clientsTableFile = clientsTableFile.arg(ContainerProps::containerTypeToString(container));
    }

    error = serverController->uploadTextFileToContainer(container, credentials, clientsTableString, clientsTableFile);
    if (error != ErrorCode::NoError) {
        logger.error() << "Failed to upload the clientsTable file to the server";
    }

    return error;
}

ErrorCode ClientManagementModel::renameClient(const int row, const QString &clientName, const DockerContainer container,
                                              const ServerCredentials &credentials,
                                              const QSharedPointer<ServerController> &serverController, bool addTimeStamp)
{
    auto client = m_clientsTable.at(row).toObject();
    auto userData = client[configKey::userData].toObject();
    userData[configKey::clientName] = clientName;
    if (addTimeStamp) {
        userData[configKey::creationDate] = QDateTime::currentDateTime().toString();
    }
    client[configKey::userData] = userData;

    m_clientsTable.replace(row, client);
    emit dataChanged(index(row, 0), index(row, 0));

    const QByteArray clientsTableString = QJsonDocument(m_clientsTable).toJson();

    QString clientsTableFile = QString("/opt/amnezia/%1/clientsTable");
    if (container == DockerContainer::OpenVpn || container == DockerContainer::ShadowSocks || container == DockerContainer::Cloak) {
        clientsTableFile = clientsTableFile.arg(ContainerProps::containerTypeToString(DockerContainer::OpenVpn));
    } else {
        clientsTableFile = clientsTableFile.arg(ContainerProps::containerTypeToString(container));
    }

    ErrorCode error = serverController->uploadTextFileToContainer(container, credentials, clientsTableString, clientsTableFile);
    if (error != ErrorCode::NoError) {
        logger.error() << "Failed to upload the clientsTable file to the server";
    }

    return error;
}

ErrorCode ClientManagementModel::revokeClient(const int row, const DockerContainer container, const ServerCredentials &credentials,
                                              const int serverIndex, const QSharedPointer<ServerController> &serverController)
{
    ErrorCode errorCode = ErrorCode::NoError;
    auto client = m_clientsTable.at(row).toObject();
    QString clientId = client.value(configKey::clientId).toString();

    if (container == DockerContainer::OpenVpn || container == DockerContainer::ShadowSocks || container == DockerContainer::Cloak) {
        errorCode = revokeOpenVpn(row, container, credentials, serverIndex, serverController);
    } else if (container == DockerContainer::WireGuard || container == DockerContainer::Awg) {
        errorCode = revokeWireGuard(row, container, credentials, serverController);
    }

    if (errorCode == ErrorCode::NoError) {
        const auto server = m_settings->server(serverIndex);
        QJsonArray containers = server.value(config_key::containers).toArray();
        for (auto i = 0; i < containers.size(); i++) {
            auto containerConfig = containers.at(i).toObject();
            auto containerType = ContainerProps::containerFromString(containerConfig.value(config_key::container).toString());
            if (containerType == container) {
                QJsonObject protocolConfig;
                if (container == DockerContainer::ShadowSocks || container == DockerContainer::Cloak) {
                    protocolConfig = containerConfig.value(ContainerProps::containerTypeToString(DockerContainer::OpenVpn)).toObject();
                } else {
                    protocolConfig = containerConfig.value(ContainerProps::containerTypeToString(containerType)).toObject();
                }

                if (protocolConfig.value(config_key::last_config).toString().contains(clientId)) {
                    emit adminConfigRevoked(container);
                }
            }
        }
    }

    return errorCode;
}

ErrorCode ClientManagementModel::revokeClient(const QJsonObject &containerConfig, const DockerContainer container,
                                              const ServerCredentials &credentials, const int serverIndex,
                                              const QSharedPointer<ServerController> &serverController)
{
    ErrorCode errorCode = ErrorCode::NoError;
    errorCode = updateModel(container, credentials, serverController);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    Proto protocol;
    if (container == DockerContainer::ShadowSocks || container == DockerContainer::Cloak) {
        protocol = Proto::OpenVpn;
    } else if (container == DockerContainer::OpenVpn || container == DockerContainer::WireGuard || container == DockerContainer::Awg) {
        protocol = ContainerProps::defaultProtocol(container);
    } else {
        return ErrorCode::NoError;
    }

    auto protocolConfig = ContainerProps::getProtocolConfigFromContainer(protocol, containerConfig);

    int row;
    bool clientExists = false;
    QString clientId = protocolConfig.value(config_key::clientId).toString();
    for (row = 0; row < rowCount(); row++) {
        auto client = m_clientsTable.at(row).toObject();
        if (clientId == client.value(configKey::clientId).toString()) {
            clientExists = true;
            break;
        }
    }
    if (!clientExists) {
        return errorCode;
    }

    if (container == DockerContainer::OpenVpn || container == DockerContainer::ShadowSocks || container == DockerContainer::Cloak) {
        errorCode = revokeOpenVpn(row, container, credentials, serverIndex, serverController);
    } else if (container == DockerContainer::WireGuard || container == DockerContainer::Awg) {
        errorCode = revokeWireGuard(row, container, credentials, serverController);
    }
    return errorCode;
}

ErrorCode ClientManagementModel::revokeOpenVpn(const int row, const DockerContainer container, const ServerCredentials &credentials,
                                               const int serverIndex, const QSharedPointer<ServerController> &serverController)
{
    auto client = m_clientsTable.at(row).toObject();
    QString clientId = client.value(configKey::clientId).toString();

    const QString getOpenVpnCertData = QString("sudo docker exec -i $CONTAINER_NAME bash -c '"
                                               "cd /opt/amnezia/openvpn ;\\"
                                               "easyrsa revoke %1 ;\\"
                                               "easyrsa gen-crl ;\\"
                                               "chmod 666 pki/crl.pem ;\\"
                                               "cp pki/crl.pem .'")
                                               .arg(clientId);

    const QString script = serverController->replaceVars(getOpenVpnCertData, serverController->genVarsForScript(credentials, container));
    ErrorCode error = serverController->runScript(credentials, script);
    if (error != ErrorCode::NoError) {
        logger.error() << "Failed to revoke the certificate";
        return error;
    }

    beginRemoveRows(QModelIndex(), row, row);
    m_clientsTable.removeAt(row);
    endRemoveRows();

    const QByteArray clientsTableString = QJsonDocument(m_clientsTable).toJson();

    QString clientsTableFile = QString("/opt/amnezia/%1/clientsTable");
    clientsTableFile = clientsTableFile.arg(ContainerProps::containerTypeToString(DockerContainer::OpenVpn));
    error = serverController->uploadTextFileToContainer(container, credentials, clientsTableString, clientsTableFile);
    if (error != ErrorCode::NoError) {
        logger.error() << "Failed to upload the clientsTable file to the server";
        return error;
    }

    return ErrorCode::NoError;
}

ErrorCode ClientManagementModel::revokeWireGuard(const int row, const DockerContainer container, const ServerCredentials &credentials,
                                                 const QSharedPointer<ServerController> &serverController)
{
    ErrorCode error = ErrorCode::NoError;

    const QString wireGuardConfigFile =
            QString("/opt/amnezia/%1/wg0.conf").arg(container == DockerContainer::WireGuard ? "wireguard" : "awg");
    const QString wireguardConfigString = serverController->getTextFileFromContainer(container, credentials, wireGuardConfigFile, error);
    if (error != ErrorCode::NoError) {
        logger.error() << "Failed to get the wg conf file from the server";
        return error;
    }

    auto client = m_clientsTable.at(row).toObject();
    QString clientId = client.value(configKey::clientId).toString();

    auto configSections = wireguardConfigString.split("[", Qt::SkipEmptyParts);
    for (auto &section : configSections) {
        if (section.contains(clientId)) {
            configSections.removeOne(section);
            break;
        }
    }
    QString newWireGuardConfig = configSections.join("[");
    newWireGuardConfig.insert(0, "[");
    error = serverController->uploadTextFileToContainer(container, credentials, newWireGuardConfig, wireGuardConfigFile);
    if (error != ErrorCode::NoError) {
        logger.error() << "Failed to upload the wg conf file to the server";
        return error;
    }

    beginRemoveRows(QModelIndex(), row, row);
    m_clientsTable.removeAt(row);
    endRemoveRows();

    const QByteArray clientsTableString = QJsonDocument(m_clientsTable).toJson();

    QString clientsTableFile = QString("/opt/amnezia/%1/clientsTable");
    if (container == DockerContainer::OpenVpn || container == DockerContainer::ShadowSocks || container == DockerContainer::Cloak) {
        clientsTableFile = clientsTableFile.arg(ContainerProps::containerTypeToString(DockerContainer::OpenVpn));
    } else {
        clientsTableFile = clientsTableFile.arg(ContainerProps::containerTypeToString(container));
    }
    error = serverController->uploadTextFileToContainer(container, credentials, clientsTableString, clientsTableFile);
    if (error != ErrorCode::NoError) {
        logger.error() << "Failed to upload the clientsTable file to the server";
        return error;
    }

    const QString script = "sudo docker exec -i $CONTAINER_NAME bash -c 'wg syncconf wg0 <(wg-quick strip %1)'";
    error = serverController->runScript(
            credentials,
            serverController->replaceVars(script.arg(wireGuardConfigFile), serverController->genVarsForScript(credentials, container)));
    if (error != ErrorCode::NoError) {
        logger.error() << "Failed to execute the command 'wg syncconf' on the server";
        return error;
    }

    return ErrorCode::NoError;
}

QHash<int, QByteArray> ClientManagementModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[ClientNameRole] = "clientName";
    roles[CreationDateRole] = "creationDate";
    roles[LatestHandshakeRole] = "latestHandshake";
    roles[DataReceivedRole] = "dataReceived";
    roles[DataSentRole] = "dataSent";
    roles[AllowedIpsRole] = "allowedIps";
    return roles;
}
