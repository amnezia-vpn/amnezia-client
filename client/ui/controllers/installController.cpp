#include "installController.h"

#include <QDesktopServices>
#include <QDir>
#include <QEventLoop>
#include <QJsonObject>
#include <QStandardPaths>

#include "core/errorstrings.h"
#include "core/servercontroller.h"
#include "utilities.h"

namespace
{
#ifdef Q_OS_WINDOWS
    QString getNextDriverLetter()
    {
        QProcess drivesProc;
        drivesProc.start("wmic logicaldisk get caption");
        drivesProc.waitForFinished();
        QString drives = drivesProc.readAll();
        qDebug() << drives;

        QString letters = "CFGHIJKLMNOPQRSTUVWXYZ";
        QString letter;
        for (int i = letters.size() - 1; i > 0; i--) {
            letter = letters.at(i);
            if (!drives.contains(letter + ":"))
                break;
        }
        if (letter == "C:") {
            // set err info
            qDebug() << "Can't find free drive letter";
            return "";
        }
        return letter;
    }
#endif
}

InstallController::InstallController(const QSharedPointer<ServersModel> &serversModel,
                                     const QSharedPointer<ContainersModel> &containersModel,
                                     const QSharedPointer<ProtocolsModel> &protocolsModel,
                                     const std::shared_ptr<Settings> &settings, QObject *parent)
    : QObject(parent),
      m_serversModel(serversModel),
      m_containersModel(containersModel),
      m_protocolModel(protocolsModel),
      m_settings(settings)
{
}

InstallController::~InstallController()
{
#ifdef Q_OS_WINDOWS
    for (QSharedPointer<QProcess> process : m_sftpMountProcesses) {
        Utils::signalCtrl(process->processId(), CTRL_C_EVENT);
        process->kill();
        process->waitForFinished();
    }
#endif
}

void InstallController::install(DockerContainer container, int port, TransportProto transportProto)
{
    QJsonObject config;
    auto mainProto = ContainerProps::defaultProtocol(container);
    for (auto protocol : ContainerProps::protocolsForContainer(container)) {
        QJsonObject containerConfig;

        if (protocol == mainProto) {
            containerConfig.insert(config_key::port, QString::number(port));
            containerConfig.insert(config_key::transport_proto,
                                   ProtocolProps::transportProtoToString(transportProto, protocol));

            if (container == DockerContainer::Sftp) {
                containerConfig.insert(config_key::userName, protocols::sftp::defaultUserName);
                containerConfig.insert(config_key::password, Utils::getRandomString(10));
            }

            config.insert(config_key::container, ContainerProps::containerToString(container));
        }
        config.insert(ProtocolProps::protoToString(protocol), containerConfig);
    }

    if (m_shouldCreateServer) {
        if (isServerAlreadyExists()) {
            return;
        }
        installServer(container, config);
    } else {
        installContainer(container, config);
    }
}

void InstallController::installServer(DockerContainer container, QJsonObject &config)
{
    ServerController serverController(m_settings);
    connect(&serverController, &ServerController::serverIsBusy, this, &InstallController::serverIsBusy);

    QMap<DockerContainer, QJsonObject> installedContainers;
    ErrorCode errorCode =
            serverController.getAlreadyInstalledContainers(m_currentlyInstalledServerCredentials, installedContainers);

    QString finishMessage = "";

    if (!installedContainers.contains(container)) {
        errorCode = serverController.setupContainer(m_currentlyInstalledServerCredentials, container, config);
        installedContainers.insert(container, config);
        finishMessage = ContainerProps::containerHumanNames().value(container) + tr(" installed successfully. ");
    } else {
        finishMessage =
                ContainerProps::containerHumanNames().value(container) + tr(" is already installed on the server. ");
    }
    if (installedContainers.size() > 1) {
        finishMessage += tr("\nAlready installed containers were found on the server. "
                            "All installed containers have been added to the application");
    }

    if (errorCode == ErrorCode::NoError) {
        QJsonObject server;
        server.insert(config_key::hostName, m_currentlyInstalledServerCredentials.hostName);
        server.insert(config_key::userName, m_currentlyInstalledServerCredentials.userName);
        server.insert(config_key::password, m_currentlyInstalledServerCredentials.secretData);
        server.insert(config_key::port, m_currentlyInstalledServerCredentials.port);
        server.insert(config_key::description, m_settings->nextAvailableServerName());

        QJsonArray containerConfigs;
        for (const QJsonObject &containerConfig : qAsConst(installedContainers)) {
            containerConfigs.append(containerConfig);
        }

        server.insert(config_key::containers, containerConfigs);
        server.insert(config_key::defaultContainer, ContainerProps::containerToString(container));

        m_serversModel->addServer(server);
        m_serversModel->setDefaultServerIndex(m_serversModel->getServersCount() - 1);

        emit installServerFinished(finishMessage);
        return;
    }

    emit installationErrorOccurred(errorString(errorCode));
}

void InstallController::installContainer(DockerContainer container, QJsonObject &config)
{
    int serverIndex = m_serversModel->getCurrentlyProcessedServerIndex();
    ServerCredentials serverCredentials =
            qvariant_cast<ServerCredentials>(m_serversModel->data(serverIndex, ServersModel::Roles::CredentialsRole));

    ServerController serverController(m_settings);
    connect(&serverController, &ServerController::serverIsBusy, this, &InstallController::serverIsBusy);

    QMap<DockerContainer, QJsonObject> installedContainers;
    ErrorCode errorCode = serverController.getAlreadyInstalledContainers(serverCredentials, installedContainers);

    QString finishMessage = "";

    if (!installedContainers.contains(container)) {
        errorCode = serverController.setupContainer(serverCredentials, container, config);
        installedContainers.insert(container, config);
        finishMessage = ContainerProps::containerHumanNames().value(container) + tr(" installed successfully. ");
    } else {
        finishMessage =
                ContainerProps::containerHumanNames().value(container) + tr(" is already installed on the server. ");
    }

    bool isInstalledContainerAddedToGui = false;

    if (errorCode == ErrorCode::NoError) {
        for (auto iterator = installedContainers.begin(); iterator != installedContainers.end(); iterator++) {
            auto modelIndex = m_containersModel->index(iterator.key());
            QJsonObject containerConfig =
                    qvariant_cast<QJsonObject>(m_containersModel->data(modelIndex, ContainersModel::Roles::ConfigRole));
            if (containerConfig.isEmpty()) {
                m_containersModel->setData(m_containersModel->index(iterator.key()), iterator.value(),
                                           ContainersModel::Roles::ConfigRole);
                if (container != iterator.key()) { // skip the newly installed container
                    isInstalledContainerAddedToGui = true;
                }
            }
        }
        if (isInstalledContainerAddedToGui) {
            finishMessage += tr("\nAlready installed containers were found on the server. "
                                "All installed containers have been added to the application");
        }

        emit installContainerFinished(finishMessage, ContainerProps::containerService(container) == ServiceType::Other);
        return;
    }

    emit installationErrorOccurred(errorString(errorCode));
}

bool InstallController::isServerAlreadyExists()
{
    for (int i = 0; i < m_serversModel->getServersCount(); i++) {
        auto modelIndex = m_serversModel->index(i);
        const ServerCredentials credentials =
                qvariant_cast<ServerCredentials>(m_serversModel->data(modelIndex, ServersModel::Roles::CredentialsRole));
        if (m_currentlyInstalledServerCredentials.hostName == credentials.hostName
            && m_currentlyInstalledServerCredentials.port == credentials.port) {
            emit serverAlreadyExists(i);
            return true;
        }
    }
    return false;
}

void InstallController::scanServerForInstalledContainers()
{
    int serverIndex = m_serversModel->getCurrentlyProcessedServerIndex();
    ServerCredentials serverCredentials =
            qvariant_cast<ServerCredentials>(m_serversModel->data(serverIndex, ServersModel::Roles::CredentialsRole));

    ServerController serverController(m_settings);

    QMap<DockerContainer, QJsonObject> installedContainers;
    ErrorCode errorCode = serverController.getAlreadyInstalledContainers(serverCredentials, installedContainers);

    if (errorCode == ErrorCode::NoError) {
        bool isInstalledContainerAddedToGui = false;

        for (auto iterator = installedContainers.begin(); iterator != installedContainers.end(); iterator++) {
            auto modelIndex = m_containersModel->index(iterator.key());
            QJsonObject containerConfig =
                    qvariant_cast<QJsonObject>(m_containersModel->data(modelIndex, ContainersModel::Roles::ConfigRole));
            if (containerConfig.isEmpty()) {
                m_containersModel->setData(m_containersModel->index(iterator.key()), iterator.value(),
                                           ContainersModel::Roles::ConfigRole);
                isInstalledContainerAddedToGui = true;
            }
        }

        emit scanServerFinished(isInstalledContainerAddedToGui);
        return;
    }

    emit installationErrorOccurred(errorString(errorCode));
}

void InstallController::updateContainer(QJsonObject config)
{
    int serverIndex = m_serversModel->getCurrentlyProcessedServerIndex();
    ServerCredentials serverCredentials =
            qvariant_cast<ServerCredentials>(m_serversModel->data(serverIndex, ServersModel::Roles::CredentialsRole));

    const DockerContainer container = ContainerProps::containerFromString(config.value(config_key::container).toString());
    auto modelIndex = m_containersModel->index(container);
    QJsonObject oldContainerConfig =
            qvariant_cast<QJsonObject>(m_containersModel->data(modelIndex, ContainersModel::Roles::ConfigRole));

    ServerController serverController(m_settings);
    connect(&serverController, &ServerController::serverIsBusy, this, &InstallController::serverIsBusy);

    auto errorCode = serverController.updateContainer(serverCredentials, container, oldContainerConfig, config);
    if (errorCode == ErrorCode::NoError) {
        m_containersModel->setData(modelIndex, config, ContainersModel::Roles::ConfigRole);
        m_protocolModel->updateModel(config);

        if ((serverIndex == m_serversModel->getDefaultServerIndex())
            && (container == m_containersModel->getDefaultContainer())) {
            emit currentContainerUpdated();
        } else {
            emit updateContainerFinished(tr("Settings updated successfully"));
        }

        return;
    }

    emit installationErrorOccurred(errorString(errorCode));
}

void InstallController::removeCurrentlyProcessedServer()
{
    int serverIndex = m_serversModel->getCurrentlyProcessedServerIndex();
    QString serverName = m_serversModel->data(serverIndex, ServersModel::Roles::NameRole).toString();

    m_serversModel->removeServer();
    emit removeCurrentlyProcessedServerFinished(tr("Server '") + serverName + tr("' was removed"));
}

void InstallController::removeAllContainers()
{
    int serverIndex = m_serversModel->getCurrentlyProcessedServerIndex();
    QString serverName = m_serversModel->data(serverIndex, ServersModel::Roles::NameRole).toString();

    ErrorCode errorCode = m_containersModel->removeAllContainers();
    if (errorCode == ErrorCode::NoError) {
        emit removeAllContainersFinished(tr("All containers from server '") + serverName + ("' have been removed"));
        return;
    }
    emit installationErrorOccurred(errorString(errorCode));
}

void InstallController::removeCurrentlyProcessedContainer()
{
    int serverIndex = m_serversModel->getCurrentlyProcessedServerIndex();
    QString serverName = m_serversModel->data(serverIndex, ServersModel::Roles::NameRole).toString();

    int container = m_containersModel->getCurrentlyProcessedContainerIndex();
    QString containerName = m_containersModel->data(container, ContainersModel::Roles::NameRole).toString();

    ErrorCode errorCode = m_containersModel->removeCurrentlyProcessedContainer();
    if (errorCode == ErrorCode::NoError) {
        emit removeCurrentlyProcessedContainerFinished(containerName + tr(" has been removed from the server '")
                                                       + serverName + "'");
        return;
    }
    emit installationErrorOccurred(errorString(errorCode));
}

QRegularExpression InstallController::ipAddressPortRegExp()
{
    return Utils::ipAddressPortRegExp();
}

QRegularExpression InstallController::ipAddressRegExp()
{
    return Utils::ipAddressRegExp();
}

void InstallController::setCurrentlyInstalledServerCredentials(const QString &hostName, const QString &userName,
                                                               const QString &secretData)
{
    m_currentlyInstalledServerCredentials.hostName = hostName;
    if (m_currentlyInstalledServerCredentials.hostName.contains(":")) {
        m_currentlyInstalledServerCredentials.port =
                m_currentlyInstalledServerCredentials.hostName.split(":").at(1).toInt();
        m_currentlyInstalledServerCredentials.hostName = m_currentlyInstalledServerCredentials.hostName.split(":").at(0);
    }
    m_currentlyInstalledServerCredentials.userName = userName;
    m_currentlyInstalledServerCredentials.secretData = secretData;
}

void InstallController::setShouldCreateServer(bool shouldCreateServer)
{
    m_shouldCreateServer = shouldCreateServer;
}

void InstallController::mountSftpDrive(const QString &port, const QString &password, const QString &username)
{
    QString mountPath;
    QString cmd;

    int serverIndex = m_serversModel->getCurrentlyProcessedServerIndex();
    ServerCredentials serverCredentials =
            qvariant_cast<ServerCredentials>(m_serversModel->data(serverIndex, ServersModel::Roles::CredentialsRole));
    QString hostname = serverCredentials.hostName;

#ifdef Q_OS_WINDOWS
    mountPath = getNextDriverLetter() + ":";
    //    QString cmd = QString("net use \\\\sshfs\\%1@x.x.x.x!%2 /USER:%1 %3")
    //            .arg(labelTftpUserNameText())
    //            .arg(labelTftpPortText())
    //            .arg(labelTftpPasswordText());

    cmd = "C:\\Program Files\\SSHFS-Win\\bin\\sshfs.exe";
#elif defined AMNEZIA_DESKTOP
    mountPath =
            QString("%1/sftp:%2:%3").arg(QStandardPaths::writableLocation(QStandardPaths::HomeLocation), hostname, port);
    QDir dir(mountPath);
    if (!dir.exists()) {
        dir.mkpath(mountPath);
    }

    cmd = "/usr/local/bin/sshfs";
#endif

#ifdef AMNEZIA_DESKTOP
    QSharedPointer<QProcess> process;
    process.reset(new QProcess());
    m_sftpMountProcesses.append(process);
    process->setProcessChannelMode(QProcess::MergedChannels);

    connect(process.get(), &QProcess::readyRead, this, [this, process, mountPath]() {
        QString s = process->readAll();
        if (s.contains("The service sshfs has been started")) {
            QDesktopServices::openUrl(QUrl("file:///" + mountPath));
        }
        qDebug() << s;
    });

    process->setProgram(cmd);

    QString args = QString("%1@%2:/ %3 "
                           "-o port=%4 "
                           "-f "
                           "-o reconnect "
                           "-o rellinks "
                           "-o fstypename=SSHFS "
                           "-o ssh_command=/usr/bin/ssh.exe "
                           "-o UserKnownHostsFile=/dev/null "
                           "-o StrictHostKeyChecking=no "
                           "-o password_stdin")
                           .arg(username, hostname, mountPath, port);

    //    args.replace("\n", " ");
    //    args.replace("\r", " ");
    // #ifndef Q_OS_WIN
    //    args.replace("reconnect-orellinks", "");
    // #endif
    process->setArguments(args.split(" ", Qt::SkipEmptyParts));
    process->start();
    process->waitForStarted(50);
    if (process->state() != QProcess::Running) {
        qDebug() << "onPushButtonSftpMountDriveClicked process not started";
        qDebug() << args;
    } else {
        process->write((password + "\n").toUtf8());
    }

    // qDebug().noquote() << "onPushButtonSftpMountDriveClicked" << args;

#endif
}

bool InstallController::checkSshConnection()
{
    ServerController serverController(m_settings);
    ErrorCode errorCode = ErrorCode::NoError;
    m_privateKeyPassphrase = "";

    if (m_currentlyInstalledServerCredentials.secretData.contains("BEGIN")
        && m_currentlyInstalledServerCredentials.secretData.contains("PRIVATE KEY")) {
        auto passphraseCallback = [this]() {
            emit passphraseRequestStarted();
            QEventLoop loop;
            QObject::connect(this, &InstallController::passphraseRequestFinished, &loop, &QEventLoop::quit);
            loop.exec();

            return m_privateKeyPassphrase;
        };

        QString decryptedPrivateKey;
        errorCode = serverController.getDecryptedPrivateKey(m_currentlyInstalledServerCredentials, decryptedPrivateKey,
                                                            passphraseCallback);
        if (errorCode == ErrorCode::NoError) {
            m_currentlyInstalledServerCredentials.secretData = decryptedPrivateKey;
        } else {
            emit installationErrorOccurred(errorString(errorCode));
            return false;
        }
    }

    QString output;
    output = serverController.checkSshConnection(m_currentlyInstalledServerCredentials, &errorCode);

    if (errorCode != ErrorCode::NoError) {
        emit installationErrorOccurred(errorString(errorCode));
        return false;
    } else {
        if (output.contains(tr("Please login as the user"))) {
            output.replace("\n", "");
            emit installationErrorOccurred(output);
            return false;
        }
    }
    return true;
}

void InstallController::setEncryptedPassphrase(QString passphrase)
{
    m_privateKeyPassphrase = passphrase;
    emit passphraseRequestFinished();
}

void InstallController::addEmptyServer()
{
    QJsonObject server;
    server.insert(config_key::hostName, m_currentlyInstalledServerCredentials.hostName);
    server.insert(config_key::userName, m_currentlyInstalledServerCredentials.userName);
    server.insert(config_key::password, m_currentlyInstalledServerCredentials.secretData);
    server.insert(config_key::port, m_currentlyInstalledServerCredentials.port);
    server.insert(config_key::description, m_settings->nextAvailableServerName());

    m_serversModel->addServer(server);
    m_serversModel->setDefaultServerIndex(m_serversModel->getServersCount() - 1);

    emit installServerFinished(tr("Server added successfully"));
}
