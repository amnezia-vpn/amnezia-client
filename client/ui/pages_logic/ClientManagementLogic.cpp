#include "ClientManagementLogic.h"

#include <QMessageBox>

#include "defines.h"
#include "core/errorstrings.h"
#include "core/servercontroller.h"
#include "ui/pages_logic/ClientInfoLogic.h"
#include "ui/models/clientManagementModel.h"
#include "ui/uilogic.h"

ClientManagementLogic::ClientManagementLogic(UiLogic *logic, QObject *parent):
    PageLogicBase(logic, parent)
{

}

void ClientManagementLogic::onUpdatePage()
{
    set_busyIndicatorIsRunning(true);

    qobject_cast<ClientManagementModel*>(uiLogic()->clientManagementModel())->clearData();
    DockerContainer selectedContainer = m_settings->defaultContainer(uiLogic()->selectedServerIndex);
    QString selectedContainerName = ContainerProps::containerHumanNames().value(selectedContainer);
    set_labelCurrentVpnProtocolText(tr("Service: ") + selectedContainerName);

    QJsonObject clients;

    auto protocols = ContainerProps::protocolsForContainer(selectedContainer);
    if (!protocols.empty()) {
        m_currentMainProtocol = protocols.front();

        ErrorCode error = getClientsList(m_settings->serverCredentials(uiLogic()->selectedServerIndex),
                                         selectedContainer, m_currentMainProtocol, clients);
        if (error != ErrorCode::NoError) {
            QMessageBox::warning(nullptr, APPLICATION_NAME,
                                 tr("An error occurred while getting the list of clients.") + "\n" + errorString(error));
            set_busyIndicatorIsRunning(false);
            return;
        }
    }
    QVector<QVariant> clientsArray;
    for (auto &clientId : clients.keys()) {
        clientsArray.push_back(clients[clientId].toObject());
    }
    qobject_cast<ClientManagementModel*>(uiLogic()->clientManagementModel())->setContent(clientsArray);

    set_busyIndicatorIsRunning(false);
}

void ClientManagementLogic::onClientItemClicked(int index)
{
    uiLogic()->pageLogic<ClientInfoLogic>()->setCurrentClientId(index);
    emit uiLogic()->goToClientInfoPage(m_currentMainProtocol);
}

ErrorCode ClientManagementLogic::getClientsList(const ServerCredentials &credentials, DockerContainer container, Proto mainProtocol, QJsonObject &clietns)
{
    ErrorCode error = ErrorCode::NoError;
    QString stdOut;
    auto cbReadStdOut = [&](const QString &data, QSharedPointer<QSsh::SshRemoteProcess> proc) {
        stdOut += data + "\n";
    };

    const QString mainProtocolString = ProtocolProps::protoToString(mainProtocol);

    const QString clientsTableFile = QString("/opt/amnezia/%1/clientsTable").arg(mainProtocolString);
    const QByteArray clientsTableString = m_serverController->getTextFileFromContainer(container, credentials, clientsTableFile, &error);
    if (error != ErrorCode::NoError) {
        return error;
    }
    QJsonObject clientsTable = QJsonDocument::fromJson(clientsTableString).object();
    int count = 0;

    if (mainProtocol == Proto::OpenVpn) {
        const QString getOpenVpnClientsList = "sudo docker exec -i $CONTAINER_NAME bash -c 'ls /opt/amnezia/openvpn/pki/issued'";
        QString script = m_serverController->replaceVars(getOpenVpnClientsList, m_serverController->genVarsForScript(credentials, container));
        error = m_serverController->runScript(credentials, script, cbReadStdOut);
        if (error != ErrorCode::NoError) {
            return error;
        }

        if (!stdOut.isEmpty()) {
            QStringList certsIds = stdOut.split("\n", Qt::SkipEmptyParts);
            certsIds.removeAll("AmneziaReq.crt");

            for (auto &openvpnCertId : certsIds) {
                openvpnCertId.replace(".crt", "");
                if (!clientsTable.contains(openvpnCertId)) {
                    stdOut.clear();
                    const QString getOpenVpnCertData = QString("sudo docker exec -i $CONTAINER_NAME bash -c 'cat /opt/amnezia/openvpn/pki/issued/%1.crt'")
                                                               .arg(openvpnCertId);
                    script = m_serverController->replaceVars(getOpenVpnCertData, m_serverController->genVarsForScript(credentials, container));
                    error = m_serverController->runScript(credentials, script, cbReadStdOut);
                    if (error != ErrorCode::NoError) {
                        return error;
                    }

                    QJsonObject client;
                    client["openvpnCertId"] = openvpnCertId;
                    client["clientName"] = QString("Client %1").arg(count);
                    client["openvpnCertData"] = stdOut;
                    clientsTable[openvpnCertId] = client;
                    count++;
                }
            }
        }
    } else if (mainProtocol == Proto::WireGuard) {
        const QString wireGuardConfigFile = "opt/amnezia/wireguard/wg0.conf";
        const QString wireguardConfigString = m_serverController->getTextFileFromContainer(container, credentials, wireGuardConfigFile, &error);
        if (error != ErrorCode::NoError) {
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
            if (!clientsTable.contains(wireguardKey)) {
                QJsonObject client;
                client["clientName"] = QString("Client %1").arg(count);
                client["wireguardPublicKey"] = wireguardKey;
                clientsTable[wireguardKey] = client;
                count++;
            }
        }
    }

    const QByteArray newClientsTableString = QJsonDocument(clientsTable).toJson();
    if (clientsTableString != newClientsTableString) {
        error = m_serverController->uploadTextFileToContainer(container, credentials, newClientsTableString, clientsTableFile);
    }

    if (error != ErrorCode::NoError) {
        return error;
    }

    clietns = clientsTable;

    return error;
}
