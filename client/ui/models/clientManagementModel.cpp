#include "clientManagementModel.h"

#include <QJsonDocument>
#include <QJsonObject>

#include "core/servercontroller.h"
#include "logger.h"

namespace
{
    Logger logger("ClientManagementModel");

    namespace configKey {
        constexpr char clientId[] = "clientId";
        constexpr char clientName[] = "clientName";
        constexpr char container[] = "container";
        constexpr char userData[] = "userData";
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
    }

    return QVariant();
}

ErrorCode ClientManagementModel::updateModel(DockerContainer container, ServerCredentials credentials)
{
    beginResetModel();
    m_clientsTable = QJsonArray();

    ServerController serverController(m_settings);

    ErrorCode error = ErrorCode::NoError;

    const QString clientsTableFile =
            QString("/opt/amnezia/%1/clientsTable").arg(ContainerProps::containerTypeToString(container));
    const QByteArray clientsTableString =
            serverController.getTextFileFromContainer(container, credentials, clientsTableFile, &error);
    if (error != ErrorCode::NoError) {
        logger.error() << "Failed to get the clientsTable file from the server";
        endResetModel();
        return error;
    }

    m_clientsTable = QJsonDocument::fromJson(clientsTableString).array();

    if (m_clientsTable.isEmpty()) {
        int count = 0;

        if (container == DockerContainer::OpenVpn || container == DockerContainer::ShadowSocks
            || container == DockerContainer::Cloak) {
            error = getOpenVpnClients(serverController, container, credentials, count);
        } else if (container == DockerContainer::WireGuard || container == DockerContainer::Awg) {
            error = getWireGuardClients(serverController, container, credentials, count);
        }
        if (error != ErrorCode::NoError) {
            endResetModel();
            return error;
        }

        const QByteArray newClientsTableString = QJsonDocument(m_clientsTable).toJson();
        if (clientsTableString != newClientsTableString) {
            error = serverController.uploadTextFileToContainer(container, credentials, newClientsTableString,
                                                               clientsTableFile);
            if (error != ErrorCode::NoError) {
                logger.error() << "Failed to upload the clientsTable file to the server";
            }
        }
    }

    endResetModel();
    return error;
}

ErrorCode ClientManagementModel::getOpenVpnClients(ServerController &serverController, DockerContainer container, ServerCredentials credentials, int &count)
{
    ErrorCode error = ErrorCode::NoError;
    QString stdOut;
    auto cbReadStdOut = [&](const QString &data, libssh::Client &) {
        stdOut += data + "\n";
        return ErrorCode::NoError;
    };

    const QString getOpenVpnClientsList =
            "sudo docker exec -i $CONTAINER_NAME bash -c 'ls /opt/amnezia/openvpn/pki/issued'";
    QString script = serverController.replaceVars(getOpenVpnClientsList,
                                                  serverController.genVarsForScript(credentials, container));
    error = serverController.runScript(credentials, script, cbReadStdOut);
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

ErrorCode ClientManagementModel::getWireGuardClients(ServerController &serverController, DockerContainer container, ServerCredentials credentials, int &count)
{
    ErrorCode error = ErrorCode::NoError;

    const QString wireGuardConfigFile =
            QString("opt/amnezia/%1/wg0.conf").arg(container == DockerContainer::WireGuard ? "wireguard" : "awg");
    const QString wireguardConfigString =
            serverController.getTextFileFromContainer(container, credentials, wireGuardConfigFile, &error);
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

bool ClientManagementModel::isClientExists(const QString &clientId)
{
    for (const QJsonValue &value : qAsConst(m_clientsTable)) {
        if (value.isObject()) {
            QJsonObject obj = value.toObject();
            if (obj.contains(configKey::clientId) && obj[configKey::clientId].toString() == clientId) {
                return true;
            }
        }
    }
    return false;
}

ErrorCode ClientManagementModel::appendClient(const QString &clientId, const QString &clientName,
                                              const DockerContainer container, ServerCredentials credentials)
{
    ErrorCode error;

    error = updateModel(container, credentials);
    if (error != ErrorCode::NoError) {
        return error;
    }

    for (int i = 0; i < m_clientsTable.size(); i++) {
        if (m_clientsTable.at(i).toObject().value(configKey::clientId) == clientId) {
            return renameClient(i, clientName, container, credentials);
        }
    }

    beginResetModel();
    QJsonObject client;
    client[configKey::clientId] = clientId;

    QJsonObject userData;
    userData[configKey::clientName] = clientName;
    client[configKey::userData] = userData;
    m_clientsTable.push_back(client);
    endResetModel();

    const QByteArray clientsTableString = QJsonDocument(m_clientsTable).toJson();

    ServerController serverController(m_settings);
    const QString clientsTableFile =
            QString("/opt/amnezia/%1/clientsTable").arg(ContainerProps::containerTypeToString(container));

    error = serverController.uploadTextFileToContainer(container, credentials, clientsTableString, clientsTableFile);
    if (error != ErrorCode::NoError) {
        logger.error() << "Failed to upload the clientsTable file to the server";
    }

    return error;
}

ErrorCode ClientManagementModel::renameClient(const int row, const QString &clientName, const DockerContainer container,
                                              ServerCredentials credentials)
{
    auto client = m_clientsTable.at(row).toObject();
    auto userData = client[configKey::userData].toObject();
    userData[configKey::clientName] = clientName;
    client[configKey::userData] = userData;

    m_clientsTable.replace(row, client);
    emit dataChanged(index(row, 0), index(row, 0));

    const QByteArray clientsTableString = QJsonDocument(m_clientsTable).toJson();

    ServerController serverController(m_settings);
    const QString clientsTableFile =
            QString("/opt/amnezia/%1/clientsTable").arg(ContainerProps::containerTypeToString(container));

    ErrorCode error =
            serverController.uploadTextFileToContainer(container, credentials, clientsTableString, clientsTableFile);
    if (error != ErrorCode::NoError) {
        logger.error() << "Failed to upload the clientsTable file to the server";
    }

    return error;
}

ErrorCode ClientManagementModel::revokeClient(const int row, const DockerContainer container,
                                              ServerCredentials credentials)
{
    if (container == DockerContainer::OpenVpn || container == DockerContainer::ShadowSocks
        || container == DockerContainer::Cloak) {
        return revokeOpenVpn(row, container, credentials);
    } else if (container == DockerContainer::WireGuard || container == DockerContainer::Awg) {
        return revokeWireGuard(row, container, credentials);
    }
    return ErrorCode::NoError;
}

ErrorCode ClientManagementModel::revokeOpenVpn(const int row, const DockerContainer container,
                                               ServerCredentials credentials)
{
    auto client = m_clientsTable.at(row).toObject();
    QString clientId = client.value(configKey::clientId).toString();

    const QString getOpenVpnCertData = QString("sudo docker exec -i $CONTAINER_NAME bash -c '"
                                               "cd /opt/amnezia/openvpn ;\\"
                                               "easyrsa revoke %1 ;\\"
                                               "easyrsa gen-crl ;\\"
                                               "cp pki/crl.pem .'")
                                               .arg(clientId);

    ServerController serverController(m_settings);
    const QString script =
            serverController.replaceVars(getOpenVpnCertData, serverController.genVarsForScript(credentials, container));
    ErrorCode error = serverController.runScript(credentials, script);
    if (error != ErrorCode::NoError) {
        logger.error() << "Failed to revoke the certificate";
        return error;
    }

    beginRemoveRows(QModelIndex(), row, row);
    m_clientsTable.removeAt(row);
    endRemoveRows();

    const QByteArray clientsTableString = QJsonDocument(m_clientsTable).toJson();

    const QString clientsTableFile =
            QString("/opt/amnezia/%1/clientsTable").arg(ContainerProps::containerTypeToString(container));
    error = serverController.uploadTextFileToContainer(container, credentials, clientsTableString, clientsTableFile);
    if (error != ErrorCode::NoError) {
        logger.error() << "Failed to upload the clientsTable file to the server";
        return error;
    }

    return ErrorCode::NoError;
}

ErrorCode ClientManagementModel::revokeWireGuard(const int row, const DockerContainer container,
                                                 ServerCredentials credentials)
{
    ErrorCode error;
    ServerController serverController(m_settings);

    const QString wireGuardConfigFile =
            QString("/opt/amnezia/%1/wg0.conf").arg(container == DockerContainer::WireGuard ? "wireguard" : "awg");
    const QString wireguardConfigString =
            serverController.getTextFileFromContainer(container, credentials, wireGuardConfigFile, &error);
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
    error = serverController.uploadTextFileToContainer(container, credentials, newWireGuardConfig, wireGuardConfigFile);
    if (error != ErrorCode::NoError) {
        logger.error() << "Failed to upload the wg conf file to the server";
        return error;
    }

    beginRemoveRows(QModelIndex(), row, row);
    m_clientsTable.removeAt(row);
    endRemoveRows();

    const QByteArray clientsTableString = QJsonDocument(m_clientsTable).toJson();

    const QString clientsTableFile =
            QString("/opt/amnezia/%1/clientsTable").arg(ContainerProps::containerTypeToString(container));
    error = serverController.uploadTextFileToContainer(container, credentials, clientsTableString, clientsTableFile);
    if (error != ErrorCode::NoError) {
        logger.error() << "Failed to upload the clientsTable file to the server";
        return error;
    }

    const QString script = "sudo docker exec -i $CONTAINER_NAME bash -c 'wg syncconf wg0 <(wg-quick strip %1)'";
    error = serverController.runScript(
            credentials,
            serverController.replaceVars(script.arg(wireGuardConfigFile),
                                         serverController.genVarsForScript(credentials, container)));
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
    return roles;
}
