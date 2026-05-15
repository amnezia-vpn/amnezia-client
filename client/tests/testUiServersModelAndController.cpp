#include <QTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QUuid>
#include <QSignalSpy>
#include <QModelIndex>

#include "core/controllers/coreController.h"
#include "core/models/serverDescription.h"
#include "core/controllers/selfhosted/importController.h"
#include "ui/models/serversModel.h"
#include "ui/models/containersModel.h"
#include "core/utils/constants/configKeys.h"

using namespace amnezia;
#include "core/utils/constants/protocolConstants.h"
#include "core/utils/containerEnum.h"
#include "core/utils/protocolEnum.h"
#include "vpnConnection.h"
#include "secureQSettings.h"

using namespace amnezia;

namespace {
int defaultServerRow(const QVector<ServerDescription> &descriptions, const QString &defaultServerId)
{
    for (int i = 0; i < descriptions.size(); ++i) {
        if (descriptions.at(i).serverId == defaultServerId) {
            return i;
        }
    }
    return -1;
}
} // namespace

class TestUiServersModelAndController : public QObject
{
    Q_OBJECT

private:
    CoreController* m_coreController;
    SecureQSettings* m_settings;

    QJsonObject createAwg2Config()
    {
        QJsonObject clientConfig;
        clientConfig[configKey::mtu] = protocols::awg::defaultMtu;
        clientConfig[configKey::junkPacketCount] = protocols::awg::defaultJunkPacketCount;
        clientConfig[configKey::junkPacketMinSize] = protocols::awg::defaultJunkPacketMinSize;
        clientConfig[configKey::junkPacketMaxSize] = protocols::awg::defaultJunkPacketMaxSize;
        clientConfig[configKey::specialJunk1] = protocols::awg::defaultSpecialJunk1;
        clientConfig[configKey::specialJunk2] = protocols::awg::defaultSpecialJunk2;
        clientConfig[configKey::specialJunk3] = protocols::awg::defaultSpecialJunk3;
        clientConfig[configKey::specialJunk4] = protocols::awg::defaultSpecialJunk4;
        clientConfig[configKey::specialJunk5] = protocols::awg::defaultSpecialJunk5;
        clientConfig[configKey::clientPrivKey] = "test_client_private_key";
        clientConfig[configKey::clientPubKey] = "test_client_public_key";
        clientConfig[configKey::serverPubKey] = "test_server_public_key";
        clientConfig[configKey::pskKey] = "test_psk_key";
        clientConfig[configKey::clientIp] = "10.8.1.2";
        clientConfig[configKey::allowedIps] = QJsonArray::fromStringList({"0.0.0.0/0"});

        QJsonObject awgConfig;
        awgConfig[configKey::lastConfig] = QString(QJsonDocument(clientConfig).toJson());
        awgConfig[configKey::port] = protocols::awg::defaultPort;
        awgConfig[configKey::transportProto] = "udp";
        awgConfig[configKey::protocolVersion] = protocols::awg::awgV2;
        awgConfig[configKey::subnetAddress] = protocols::wireguard::defaultSubnetAddress;
        awgConfig[configKey::junkPacketCount] = protocols::awg::defaultJunkPacketCount;
        awgConfig[configKey::junkPacketMinSize] = protocols::awg::defaultJunkPacketMinSize;
        awgConfig[configKey::junkPacketMaxSize] = protocols::awg::defaultJunkPacketMaxSize;
        awgConfig[configKey::initPacketJunkSize] = protocols::awg::defaultInitPacketJunkSize;
        awgConfig[configKey::responsePacketJunkSize] = protocols::awg::defaultResponsePacketJunkSize;
        awgConfig[configKey::cookieReplyPacketJunkSize] = protocols::awg::defaultCookieReplyPacketJunkSize;
        awgConfig[configKey::transportPacketJunkSize] = protocols::awg::defaultTransportPacketJunkSize;
        awgConfig[configKey::initPacketMagicHeader] = protocols::awg::defaultInitPacketMagicHeader;
        awgConfig[configKey::responsePacketMagicHeader] = protocols::awg::defaultResponsePacketMagicHeader;
        awgConfig[configKey::underloadPacketMagicHeader] = protocols::awg::defaultUnderloadPacketMagicHeader;
        awgConfig[configKey::transportPacketMagicHeader] = protocols::awg::defaultTransportPacketMagicHeader;
        awgConfig[configKey::specialJunk1] = protocols::awg::defaultSpecialJunk1;
        awgConfig[configKey::specialJunk2] = protocols::awg::defaultSpecialJunk2;
        awgConfig[configKey::specialJunk3] = protocols::awg::defaultSpecialJunk3;
        awgConfig[configKey::specialJunk4] = protocols::awg::defaultSpecialJunk4;
        awgConfig[configKey::specialJunk5] = protocols::awg::defaultSpecialJunk5;
        awgConfig[configKey::isThirdPartyConfig] = true;

        QJsonObject container;
        container[configKey::container] = "amnezia-awg";
        container[configKey::awg] = awgConfig;

        QJsonArray containers;
        containers.append(container);

        QJsonObject config;
        config[configKey::containers] = containers;
        config[configKey::defaultContainer] = "amnezia-awg";
        config[configKey::description] = "AWG2 Test Server";
        config[configKey::hostName] = "test.example.com";

        return config;
    }

    QJsonObject createServerDescriptionTestConfig(bool withAmneziaDns)
    {
        QJsonObject config = createAwg2Config();
        config[configKey::description] = "Server 1";
        if (withAmneziaDns) {
            config[configKey::dns1] = protocols::dns::amneziaDnsIp;
        }
        return config;
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
            m_coreController->m_serversModel->updateModel(QVector<ServerDescription>(), -1);
        }
    }

    void testUiServersModelAndControllerRoles() {
        QJsonObject testConfig = createAwg2Config();
        
        QSignalSpy importFinishedSpy(m_coreController->m_importCoreController, &ImportController::importFinished);
        
        m_coreController->m_importCoreController->importConfig(testConfig);
        
        QVERIFY2(importFinishedSpy.count() == 1, "importFinished signal should be emitted");
        QVERIFY2(m_coreController->m_serversRepository->serversCount() == 1, "Server should be imported");
        
        int serverIndex = m_coreController->m_serversRepository->defaultServerIndex();
        QVERIFY2(serverIndex == 0, "Default server index should be 0");
        
        if (m_coreController->m_serversModel) {
            QVERIFY2(m_coreController->m_serversModel->rowCount() == 1, "ServersModel should have 1 row");
            
            QModelIndex serverModelIndex = m_coreController->m_serversModel->index(0, 0);
            QVERIFY2(serverModelIndex.isValid(), "Server model index should be valid");
            
            QString serverName = m_coreController->m_serversModel->data(serverModelIndex, ServersModel::NameRole).toString();
            QVERIFY2(serverName == "AWG2 Test Server", QString("Server name should be 'AWG2 Test Server', got '%1'").arg(serverName).toUtf8().constData());
            
            QString serverDescription = m_coreController->m_serversModel->data(serverModelIndex, ServersModel::ServerDescriptionRole).toString();
            QVERIFY2(serverDescription.contains("test.example.com"), QString("Server description should contain hostname, got '%1'").arg(serverDescription).toUtf8().constData());
            
            QString hostName = m_coreController->m_serversModel->data(serverModelIndex, ServersModel::HostNameRole).toString();
            QVERIFY2(hostName == "test.example.com", "Host name should match");
            
            bool isDefault = m_coreController->m_serversModel->data(serverModelIndex, ServersModel::IsDefaultRole).toBool();
            QVERIFY2(isDefault == true, "Server should be default");
            
            bool hasInstalledContainers = m_coreController->m_serversModel->data(serverModelIndex, ServersModel::HasInstalledContainers).toBool();
            QVERIFY2(hasInstalledContainers == true, "Server should have installed containers");
            
            bool hasWriteAccess = m_coreController->m_serversModel->data(serverModelIndex, ServersModel::HasWriteAccessRole).toBool();
            QVERIFY2(hasWriteAccess == false, "Server should not have write access for imported config");
            
            int defaultContainerRole = m_coreController->m_serversModel->data(serverModelIndex, ServersModel::DefaultContainerRole).toInt();
            DockerContainer expectedContainer = DockerContainer::Awg;
            QVERIFY2(defaultContainerRole == static_cast<int>(expectedContainer), "Default container should be Awg");
        }
        
        if (m_coreController->m_serversUiController) {
            m_coreController->m_serversUiController->setProcessedServerId(
                    m_coreController->m_serversUiController->getServerId(0));

            QString hostName = "test.example.com";
            
            QString collapsedDescription = m_coreController->m_serversUiController->getDefaultServerDescriptionCollapsed();
            QString expectedCollapsed = "AmneziaWG (version 2) | " + hostName;
            QVERIFY2(collapsedDescription == expectedCollapsed, 
                     QString("Collapsed description should be '%1', got '%2'").arg(expectedCollapsed, collapsedDescription).toUtf8().constData());
            
            QString expandedDescription = m_coreController->m_serversUiController->getDefaultServerDescriptionExpanded();
            QString expectedExpanded = hostName;
            QVERIFY2(expandedDescription == expectedExpanded, 
                     QString("Expanded description should be '%1', got '%2'").arg(expectedExpanded, expandedDescription).toUtf8().constData());
        }
        
        if (m_coreController->m_containersModel) {
            
            int awgContainerIndex = -1;
            for (int i = 0; i < ContainerUtils::allContainers().size(); ++i) {
                DockerContainer container = ContainerUtils::allContainers().at(i);
                if (container == DockerContainer::Awg) {
                    awgContainerIndex = i;
                    break;
                }
            }
            
            QVERIFY2(awgContainerIndex >= 0, "Awg container index should be found");
            
            QModelIndex containerModelIndex = m_coreController->m_containersModel->index(awgContainerIndex, 0);
            QVERIFY2(containerModelIndex.isValid(), "Container model index should be valid");
            
            bool isInstalled = m_coreController->m_containersModel->data(containerModelIndex, ContainersModel::IsInstalledRole).toBool();
            QVERIFY2(isInstalled == true, "Awg container should be installed");
            
            bool isVpnContainer = m_coreController->m_containersModel->data(containerModelIndex, ContainersModel::IsVpnContainerRole).toBool();
            QVERIFY2(isVpnContainer == true, "Awg container should be VPN container");
            
            QString containerName = m_coreController->m_containersModel->data(containerModelIndex, ContainersModel::NameRole).toString();
            QString expectedContainerName = ContainerUtils::containerHumanNames().value(DockerContainer::Awg);
            QVERIFY2(containerName == expectedContainerName, QString("Container name should be '%1', got '%2'").arg(expectedContainerName, containerName).toUtf8().constData());
            
            QString containerDescription = m_coreController->m_containersModel->data(containerModelIndex, ContainersModel::DescriptionRole).toString();
            QString expectedDescription = ContainerUtils::containerDescriptions().value(DockerContainer::Awg);
            QVERIFY2(containerDescription == expectedDescription, QString("Container description should match, got '%1'").arg(containerDescription).toUtf8().constData());
            
            QString detailedDescription = m_coreController->m_containersModel->data(containerModelIndex, ContainersModel::DetailedDescriptionRole).toString();
            QString expectedDetailedDescription = ContainerUtils::containerDetailedDescriptions().value(DockerContainer::Awg);
            QVERIFY2(detailedDescription == expectedDetailedDescription, QString("Container detailed description should match, got '%1'").arg(detailedDescription).toUtf8().constData());
            
            int serviceType = m_coreController->m_containersModel->data(containerModelIndex, ContainersModel::ServiceTypeRole).toInt();
            QVERIFY2(serviceType == static_cast<int>(ProtocolEnumNS::ServiceType::Vpn), "Service type should be Vpn");
            
            bool isSupported = m_coreController->m_containersModel->data(containerModelIndex, ContainersModel::IsSupportedRole).toBool();
            QVERIFY2(isSupported == true, "Container should be supported");
            
            bool isShareable = m_coreController->m_containersModel->data(containerModelIndex, ContainersModel::IsShareableRole).toBool();
            QVERIFY2(isShareable == true, "Container should be shareable");
            
            QJsonObject containerConfig = m_coreController->m_containersModel->data(containerModelIndex, ContainersModel::ConfigRole).toJsonObject();
            QVERIFY2(!containerConfig.isEmpty(), "Container config should not be empty");
            QVERIFY2(containerConfig.value(configKey::container).toString() == "amnezia-awg", "Container config should have correct container type");
            
            QJsonObject awgProtocolConfig = containerConfig.value(configKey::awg).toObject();
            QVERIFY2(!awgProtocolConfig.isEmpty(), "AWG protocol config should not be empty");
            
            QString protocolVersion = awgProtocolConfig.value(configKey::protocolVersion).toString();
            QVERIFY2(protocolVersion == protocols::awg::awgV2, QString("Protocol version should be '%1', got '%2'").arg(protocols::awg::awgV2, protocolVersion).toUtf8().constData());
            
            QString port = awgProtocolConfig.value(configKey::port).toString();
            QVERIFY2(port == protocols::awg::defaultPort, QString("Port should be '%1', got '%2'").arg(protocols::awg::defaultPort, port).toUtf8().constData());
            
            QString subnetAddress = awgProtocolConfig.value(configKey::subnetAddress).toString();
            QVERIFY2(subnetAddress == protocols::wireguard::defaultSubnetAddress, QString("Subnet address should be '%1', got '%2'").arg(protocols::wireguard::defaultSubnetAddress, subnetAddress).toUtf8().constData());
            
            bool isThirdParty = m_coreController->m_containersModel->data(containerModelIndex, ContainersModel::IsThirdPartyConfigRole).toBool();
            QVERIFY2(isThirdParty == true, "Imported config should be third party config");
            
            DockerContainer dockerContainer = static_cast<DockerContainer>(m_coreController->m_containersModel->data(containerModelIndex, ContainersModel::DockerContainerRole).toInt());
            QVERIFY2(dockerContainer == DockerContainer::Awg, "Docker container should be Awg");
            
            QString containerString = m_coreController->m_containersModel->data(containerModelIndex, ContainersModel::ContainerStringRole).toString();
            QVERIFY2(containerString == "amnezia-awg", "Container string should be amnezia-awg");
        }
    }

    void testServerDescriptionFormat() {
        QSignalSpy importFinishedSpy(m_coreController->m_importCoreController, &ImportController::importFinished);

        QJsonObject configNoDns = createServerDescriptionTestConfig(false);
        m_coreController->m_importCoreController->importConfig(configNoDns);
        QVERIFY2(importFinishedSpy.count() == 1, "importFinished should be emitted");
        m_coreController->m_appSettingsRepository->setUseAmneziaDns(false);
        QVector<ServerDescription> descriptionsNoDns = m_coreController->m_serversController->buildServerDescriptions(
            m_coreController->m_appSettingsRepository->useAmneziaDns());
        const QString defIdNoDns = m_coreController->m_serversRepository->defaultServerId();
        m_coreController->m_serversModel->updateModel(descriptionsNoDns, defaultServerRow(descriptionsNoDns, defIdNoDns));

        QString descNoDns = m_coreController->m_serversModel->data(
            m_coreController->m_serversModel->index(0, 0), ServersModel::ServerDescriptionRole).toString();
        QVERIFY2(descNoDns == "test.example.com",
                 QString("Without Amnezia DNS expected 'test.example.com', got '%1'").arg(descNoDns).toUtf8().constData());

        m_coreController->m_serversRepository->clearServers();
        if (m_coreController->m_serversRepository->serversCount() > 0) {
            m_coreController->m_serversRepository->setDefaultServer(m_coreController->m_serversRepository->serverIdAt(0));
        }

        QJsonObject configWithDns = createServerDescriptionTestConfig(true);
        m_coreController->m_importCoreController->importConfig(configWithDns);
        QVERIFY2(m_coreController->m_serversRepository->serversCount() == 1, "Server should be imported");
        m_coreController->m_appSettingsRepository->setUseAmneziaDns(true);
        QVector<ServerDescription> descriptionsWithDns = m_coreController->m_serversController->buildServerDescriptions(
            m_coreController->m_appSettingsRepository->useAmneziaDns());
        const QString defIdWithDns = m_coreController->m_serversRepository->defaultServerId();
        m_coreController->m_serversModel->updateModel(descriptionsWithDns, defaultServerRow(descriptionsWithDns, defIdWithDns));

        QString descWithDns = m_coreController->m_serversModel->data(
            m_coreController->m_serversModel->index(0, 0), ServersModel::ServerDescriptionRole).toString();
        QVERIFY2(descWithDns == "Amnezia DNS | test.example.com",
                 QString("With Amnezia DNS expected 'Amnezia DNS | test.example.com', got '%1'").arg(descWithDns).toUtf8().constData());
    }
};

QTEST_MAIN(TestUiServersModelAndController)
#include "testUiServersModelAndController.moc"
