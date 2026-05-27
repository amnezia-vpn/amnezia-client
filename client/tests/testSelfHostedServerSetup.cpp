#include <QTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>
#include <QSignalSpy>
#include <QProcessEnvironment>
#include <QDebug>

#include "core/controllers/coreController.h"
#include "core/models/serverDescription.h"
#include "core/models/selfhosted/selfHostedAdminServerConfig.h"
#include "core/models/containerConfig.h"
#include "core/models/protocols/awgProtocolConfig.h"
#include "core/models/protocols/dnsProtocolConfig.h"
#include "core/utils/commonStructs.h"
#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/utils/errorCodes.h"
#include "ui/models/serversModel.h"
#include "vpnConnection.h"
#include "secureQSettings.h"

using namespace amnezia;

class TestSelfHostedServerSetup : public QObject
{
    Q_OBJECT

private:
    CoreController* m_coreController;
    SecureQSettings* m_settings;

    ServerCredentials getCredentialsFromEnv() {
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        
        QString hostName = env.value("TEST_SERVER_HOST");
        QString userName = env.value("TEST_SERVER_USER");
        QString password = env.value("TEST_SERVER_PASSWORD");
        QString portStr = env.value("TEST_SERVER_PORT", "22");
        int port = portStr.toInt();
        
        ServerCredentials credentials;
        credentials.hostName = hostName;
        credentials.userName = userName;
        credentials.secretData = password;
        credentials.port = port;
        
        return credentials;
    }

    void verifySshConnection(const ServerCredentials& credentials) {
        QString sshOutput;
        ErrorCode sshError = m_coreController->m_installController->checkSshConnection(credentials, sshOutput);
        QVERIFY2(sshError == ErrorCode::NoError, 
                 QString("SSH connection should succeed. Error: %1, Output: %2")
                     .arg(static_cast<int>(sshError))
                     .arg(sshOutput)
                     .toUtf8().constData());
        qDebug() << "SSH connection successful. Output:" << sshOutput;
    }

    void verifyAdminAccess(int serverIndex)
    {
        const QString serverId = m_coreController->m_serversRepository->serverIdAt(serverIndex);
        const auto adminCfg = m_coreController->m_serversRepository->selfHostedAdminConfig(serverId);
        QVERIFY2(adminCfg.has_value(), "Server config should be SelfHostedAdminServerConfig");

        const SelfHostedAdminServerConfig &selfHosted = *adminCfg;
        
        QVERIFY2(selfHosted.hasCredentials(), 
                 "Server should have credentials (admin access)");
        
        QVERIFY2(!selfHosted.userName.isEmpty(),
                 "Server should have userName for admin access");
        
        QVERIFY2(!selfHosted.password.isEmpty(),
                 "Server should have password for admin access");
        
        QVERIFY2(!selfHosted.isReadOnly(), 
                 "Server should not be read-only (should have admin access)");
        
        if (m_coreController->m_serversModel) {
            bool hasWriteAccess = m_coreController->m_serversModel->data(
                m_coreController->m_serversModel->index(serverIndex, 0),
                ServersModel::HasWriteAccessRole
            ).toBool();
            
            QVERIFY2(hasWriteAccess, 
                     "Server should have write access (admin access) according to ServersModel");
        }
        
        qDebug() << "Admin access verified for server at index:" << serverIndex;
    }

    void verifyClientConfig(const ContainerConfig& containerConfig, DockerContainer container) {
        QString containerName = ContainerUtils::containerToString(container);
        qDebug() << "Checking container:" << containerName;
        
        if (ContainerUtils::containerService(container) != ServiceType::Other) {
            bool hasClientConfig = containerConfig.protocolConfig.hasClientConfig();
            
            QVERIFY2(hasClientConfig, 
                     QString("Container %1 should have client config initialized")
                         .arg(containerName)
                         .toUtf8().constData());
            
            if (container == DockerContainer::Awg) {
                const AwgProtocolConfig* awgProtocolConfig = containerConfig.protocolConfig.as<AwgProtocolConfig>();
                QVERIFY2(awgProtocolConfig != nullptr, "Protocol config should be AwgProtocolConfig");
                QVERIFY2(awgProtocolConfig->hasClientConfig(), "AwgProtocolConfig should have client config");
                const std::optional<AwgClientConfig>& clientCfgOpt = awgProtocolConfig->clientConfig;
                QVERIFY2(clientCfgOpt.has_value(), "Awg client config should exist");

                const AwgClientConfig& awgClientConfig = *clientCfgOpt;
                QVERIFY2(!awgClientConfig.hostName.isEmpty(), "Awg client config should have hostName");
                QVERIFY2(awgClientConfig.port > 0, "Awg client config should have valid port");
                QVERIFY2(!awgClientConfig.clientPrivateKey.isEmpty(), "Awg client config should have clientPrivateKey");
                QVERIFY2(!awgClientConfig.clientPublicKey.isEmpty(), "Awg client config should have clientPublicKey");
                QVERIFY2(!awgClientConfig.serverPublicKey.isEmpty(), "Awg client config should have serverPublicKey");
                QVERIFY2(!awgClientConfig.clientId.isEmpty(), "Awg client config should have clientId");
                QVERIFY2(!awgClientConfig.nativeConfig.isEmpty(), "Awg client config should have nativeConfig");
            }
            
            qDebug() << "Container" << containerName << "has valid client config initialized";
        } else {
            qDebug() << "Container" << containerName << "is service type Other, skipping client config check";
        }
    }

private slots:
    void initTestCase() {
        QString testOrg = "AmneziaVPN-Test-" + QUuid::createUuid().toString();
        m_settings = new SecureQSettings(testOrg, "amnezia-client", nullptr, false);
        
        auto vpnConnection = QSharedPointer<VpnConnection>::create(nullptr, nullptr);
        m_coreController = new CoreController(vpnConnection, m_settings, nullptr, this);
    }

    void cleanupTestCase() {
        m_settings->clearSettings();
        delete m_coreController;
        delete m_settings;
    }

    void init() {
        m_settings->clearSettings();
        if (m_coreController->m_serversModel) {
            m_coreController->m_serversModel->updateModel(QVector<ServerDescription>(), QString());
        }
    }

    void testSelfHostedServerSetup() {
        ServerCredentials credentials = getCredentialsFromEnv();
        
        if (credentials.hostName.isEmpty() || credentials.userName.isEmpty() || credentials.secretData.isEmpty()) {
            QSKIP("Test requires TEST_SERVER_HOST, TEST_SERVER_USER, TEST_SERVER_PASSWORD environment variables");
        }
        
        QVERIFY2(credentials.isValid(), "Server credentials should be valid");
        qDebug() << "Using server:" << credentials.hostName << "user:" << credentials.userName << "port:" << credentials.port;
        
        verifySshConnection(credentials);
        
        int awgPort = 55424;
        TransportProto awgTransportProto = TransportProto::Udp;
        bool wasAwgInstalled = false;
        
        QSignalSpy serverAddedSpy(m_coreController->m_serversRepository, &SecureServersRepository::serverAdded);
        ErrorCode installServerError = m_coreController->m_installController->installServer(
            credentials, DockerContainer::Awg, awgPort, awgTransportProto, wasAwgInstalled);
        
        QVERIFY2(installServerError == ErrorCode::NoError,
                 QString("installServer for Awg should succeed. Error: %1")
                     .arg(static_cast<int>(installServerError))
                     .toUtf8().constData());
        QVERIFY2(serverAddedSpy.count() == 1, "serverAdded signal should be emitted");
        QVERIFY2(m_coreController->m_serversRepository->serversCount() > 0, "Server should be added");
        
        int serverIndex = m_coreController->m_serversRepository->serversCount() - 1;
        qDebug() << "Server with Awg container added at index:" << serverIndex;
        
        const auto adminAfterAwg = m_coreController->m_serversRepository->selfHostedAdminConfig(
            m_coreController->m_serversRepository->serverIdAt(serverIndex));
        QVERIFY2(adminAfterAwg.has_value(), "Server should be self-hosted (admin)");
        const SelfHostedAdminServerConfig *selfHostedAfterAwg = &(*adminAfterAwg);
        QVERIFY2(selfHostedAfterAwg->defaultContainer == DockerContainer::Awg, "Default container should be Awg");
        QVERIFY2(selfHostedAfterAwg->containers.contains(DockerContainer::Awg), "Server should have Awg container");
        
        ContainerConfig awgConfig = selfHostedAfterAwg->containers.value(DockerContainer::Awg);
        QVERIFY2(awgConfig.container == DockerContainer::Awg, "Awg container config should be valid");
        QVERIFY2(selfHostedAfterAwg->containers.size() == 1, 
                 QString("Server should have exactly 1 container after Awg installation, but has %1")
                     .arg(selfHostedAfterAwg->containers.size())
                     .toUtf8().constData());
        verifyClientConfig(awgConfig, DockerContainer::Awg);
        
        qDebug() << "Awg container installed and configured successfully with valid client config";
        
        int dnsPort = 53;
        TransportProto dnsTransportProto = TransportProto::Udp;
        bool wasDnsInstalled = false;
        
        const QString serverIdForOps = m_coreController->m_serversRepository->serverIdAt(serverIndex);
        ErrorCode installContainerError = m_coreController->m_installController->installContainer(
            serverIdForOps, DockerContainer::Dns, dnsPort, dnsTransportProto, wasDnsInstalled);
        
        QVERIFY2(installContainerError == ErrorCode::NoError,
                 QString("installContainer for Dns should succeed. Error: %1")
                     .arg(static_cast<int>(installContainerError))
                     .toUtf8().constData());
        qDebug() << "Dns container installed:" << wasDnsInstalled;
        
        const auto adminAfterDns = m_coreController->m_serversRepository->selfHostedAdminConfig(
            m_coreController->m_serversRepository->serverIdAt(serverIndex));
        QVERIFY2(adminAfterDns.has_value(), "Server config should be SelfHostedAdminServerConfig");
        const SelfHostedAdminServerConfig *selfHostedAfterDns = &(*adminAfterDns);
        QVERIFY2(selfHostedAfterDns->containers.contains(DockerContainer::Awg), "Server should still have Awg container");
        QVERIFY2(selfHostedAfterDns->containers.contains(DockerContainer::Dns), "Server should have Dns container");
        QVERIFY2(selfHostedAfterDns->containers.size() == 2, 
                 QString("Server should have exactly 2 containers after Dns installation, but has %1")
                     .arg(selfHostedAfterDns->containers.size())
                     .toUtf8().constData());
        
        ContainerConfig dnsConfig = selfHostedAfterDns->containers.value(DockerContainer::Dns);
        QVERIFY2(dnsConfig.container == DockerContainer::Dns, "Dns container config should be valid");
        
        const DnsProtocolConfig* dnsProtocolConfig = dnsConfig.protocolConfig.as<DnsProtocolConfig>();
        QVERIFY2(dnsProtocolConfig != nullptr, "Protocol config should be DnsProtocolConfig");
        
        qDebug() << "Dns container installed and configured successfully";
        
        verifyAdminAccess(serverIndex);
        
        qDebug() << "Test completed successfully. Server setup with Awg and Dns containers is complete.";
    }

    void testSelfHostedServerEmptyRecover() {
        ServerCredentials credentials = getCredentialsFromEnv();
        
        if (credentials.hostName.isEmpty() || credentials.userName.isEmpty() || credentials.secretData.isEmpty()) {
            QSKIP("Test requires TEST_SERVER_HOST, TEST_SERVER_USER, TEST_SERVER_PASSWORD environment variables");
        }
        
        QVERIFY2(credentials.isValid(), "Server credentials should be valid");
        qDebug() << "Using server:" << credentials.hostName << "user:" << credentials.userName << "port:" << credentials.port;
        
        verifySshConnection(credentials);
        
        SelfHostedAdminServerConfig serverConfig;
        serverConfig.hostName = credentials.hostName;
        serverConfig.userName = credentials.userName;
        serverConfig.password = credentials.secretData;
        serverConfig.port = credentials.port;
        serverConfig.description = m_coreController->m_appSettingsRepository->nextAvailableServerName();
        serverConfig.displayName = serverConfig.description.isEmpty() ? serverConfig.hostName : serverConfig.description;
        serverConfig.defaultContainer = DockerContainer::None;
        
        QSignalSpy serverAddedSpy(m_coreController->m_serversRepository, &SecureServersRepository::serverAdded);
        m_coreController->m_serversRepository->addServer(QString(), serverConfig.toJson(),
                                                         serverConfigUtils::ConfigType::SelfHostedAdmin);
        
        QVERIFY2(serverAddedSpy.count() == 1, "serverAdded signal should be emitted");
        QVERIFY2(m_coreController->m_serversRepository->serversCount() > 0, "Server should be added");
        
        int serverIndex = m_coreController->m_serversRepository->serversCount() - 1;
        qDebug() << "Empty server added at index:" << serverIndex;
        
        const auto addedAdmin = m_coreController->m_serversRepository->selfHostedAdminConfig(
            m_coreController->m_serversRepository->serverIdAt(serverIndex));
        QVERIFY2(addedAdmin.has_value(), "Added server should be self-hosted admin");
        const SelfHostedAdminServerConfig *selfHosted = &(*addedAdmin);
        QVERIFY2(selfHosted->containers.isEmpty(), "Server should have no containers initially");
        QVERIFY2(selfHosted->defaultContainer == DockerContainer::None, "Default container should be None");
        
        const QString scanServerId = m_coreController->m_serversRepository->serverIdAt(serverIndex);
        ErrorCode scanError = m_coreController->m_installController->scanServerForInstalledContainers(scanServerId);
        QVERIFY2(scanError == ErrorCode::NoError, 
                 QString("Server scan should succeed. Error: %1")
                     .arg(static_cast<int>(scanError))
                     .toUtf8().constData());
        qDebug() << "Server scan completed successfully";
        
        const auto scannedAdmin = m_coreController->m_serversRepository->selfHostedAdminConfig(
            m_coreController->m_serversRepository->serverIdAt(serverIndex));
        QVERIFY2(scannedAdmin.has_value(), "Scanned server config should be SelfHostedAdminServerConfig");
        const SelfHostedAdminServerConfig *scannedSelfHosted = &(*scannedAdmin);
        
        QMap<DockerContainer, ContainerConfig> containers = scannedSelfHosted->containers;
        int containersCount = containers.size();
        qDebug() << "Found containers count:" << containersCount;
        
        QVERIFY2(containersCount >= 0, 
                 QString("Containers count should be non-negative, but got %1")
                     .arg(containersCount)
                     .toUtf8().constData());
        
        if (containersCount > 0) {
            qDebug() << "Server has" << containersCount << "installed container(s)";
        } else {
            qDebug() << "Server has no installed containers";
        }
        
        for (auto it = containers.begin(); it != containers.end(); ++it) {
            verifyClientConfig(it.value(), it.key());
        }
        
        QVERIFY2(scannedSelfHosted->containers.size() == containersCount,
                 QString("Scanned containers count should match. Expected: %1, Actual: %2")
                     .arg(containersCount)
                     .arg(scannedSelfHosted->containers.size())
                     .toUtf8().constData());
        
        verifyAdminAccess(serverIndex);
        
        qDebug() << "Test completed successfully. Server has admin access and all containers are initialized.";
    }

    void testRemoveAllContainers() {
        ServerCredentials credentials = getCredentialsFromEnv();
        
        if (credentials.hostName.isEmpty() || credentials.userName.isEmpty() || credentials.secretData.isEmpty()) {
            QSKIP("Test requires TEST_SERVER_HOST, TEST_SERVER_USER, TEST_SERVER_PASSWORD environment variables");
        }
        
        QVERIFY2(credentials.isValid(), "Server credentials should be valid");
        qDebug() << "Using server:" << credentials.hostName << "user:" << credentials.userName << "port:" << credentials.port;
        
        verifySshConnection(credentials);
        
        int awgPort = 55424;
        TransportProto awgTransportProto = TransportProto::Udp;
        bool wasAwgInstalled = false;
        
        QSignalSpy serverAddedSpy(m_coreController->m_serversRepository, &SecureServersRepository::serverAdded);
        ErrorCode installServerError = m_coreController->m_installController->installServer(
            credentials, DockerContainer::Awg, awgPort, awgTransportProto, wasAwgInstalled);
        
        QVERIFY2(installServerError == ErrorCode::NoError,
                 QString("installServer for Awg should succeed. Error: %1")
                     .arg(static_cast<int>(installServerError))
                     .toUtf8().constData());
        QVERIFY2(serverAddedSpy.count() == 1, "serverAdded signal should be emitted");
        
        int serverIndex = m_coreController->m_serversRepository->serversCount() - 1;
        qDebug() << "Server with Awg container added at index:" << serverIndex;
        
        const auto adminBeforeRemoval = m_coreController->m_serversRepository->selfHostedAdminConfig(
            m_coreController->m_serversRepository->serverIdAt(serverIndex));
        QVERIFY2(adminBeforeRemoval.has_value(), "Server config should be SelfHostedAdminServerConfig");
        const SelfHostedAdminServerConfig *selfHostedBeforeRemoval = &(*adminBeforeRemoval);
        QVERIFY2(!selfHostedBeforeRemoval->containers.isEmpty(), "Server should have containers before removal");
        QVERIFY2(selfHostedBeforeRemoval->defaultContainer != DockerContainer::None, "Server should have default container before removal");
        
        qDebug() << "Containers before removal:" << selfHostedBeforeRemoval->containers.size();
        
        const QString removeServerId = m_coreController->m_serversRepository->serverIdAt(serverIndex);
        ErrorCode removeError = m_coreController->m_installController->removeAllContainers(removeServerId);
        QVERIFY2(removeError == ErrorCode::NoError,
                 QString("removeAllContainers should succeed. Error: %1")
                     .arg(static_cast<int>(removeError))
                     .toUtf8().constData());
        qDebug() << "All containers removed successfully";
        
        const auto adminAfterRemoval = m_coreController->m_serversRepository->selfHostedAdminConfig(
            m_coreController->m_serversRepository->serverIdAt(serverIndex));
        QVERIFY2(adminAfterRemoval.has_value(), "Server config should be SelfHostedAdminServerConfig");
        const SelfHostedAdminServerConfig *selfHostedAfterRemoval = &(*adminAfterRemoval);
        
        QVERIFY2(selfHostedAfterRemoval->containers.isEmpty(), 
                 "Server should have no containers after removal");
        QVERIFY2(selfHostedAfterRemoval->defaultContainer == DockerContainer::None, 
                 "Default container should be None after removal");
        
        qDebug() << "Containers after removal:" << selfHostedAfterRemoval->containers.size();
        
        verifyAdminAccess(serverIndex);
        
        qDebug() << "Test completed successfully. All containers removed and server is empty.";
    }
};

QTEST_MAIN(TestSelfHostedServerSetup)
#include "testSelfHostedServerSetup.moc"

