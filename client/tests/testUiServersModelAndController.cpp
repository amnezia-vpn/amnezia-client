#include <QTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QUuid>
#include <QSignalSpy>
#include <QModelIndex>

#include "core/controllers/coreController.h"
#include "core/models/serverConfig.h"
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

class TestUiServersModelAndController : public QObject
{
    Q_OBJECT

private:
    CoreController* m_coreController;
    SecureQSettings* m_settings;

    QJsonObject createAwg2Config()
    {
        QJsonObject clientConfig;
        clientConfig[config_key::mtu] = protocols::awg::defaultMtu;
        clientConfig[config_key::junkPacketCount] = protocols::awg::defaultJunkPacketCount;
        clientConfig[config_key::junkPacketMinSize] = protocols::awg::defaultJunkPacketMinSize;
        clientConfig[config_key::junkPacketMaxSize] = protocols::awg::defaultJunkPacketMaxSize;
        clientConfig[config_key::specialJunk1] = protocols::awg::defaultSpecialJunk1;
        clientConfig[config_key::specialJunk2] = protocols::awg::defaultSpecialJunk2;
        clientConfig[config_key::specialJunk3] = protocols::awg::defaultSpecialJunk3;
        clientConfig[config_key::specialJunk4] = protocols::awg::defaultSpecialJunk4;
        clientConfig[config_key::specialJunk5] = protocols::awg::defaultSpecialJunk5;
        clientConfig[config_key::client_priv_key] = "test_client_private_key";
        clientConfig[config_key::client_pub_key] = "test_client_public_key";
        clientConfig[config_key::server_pub_key] = "test_server_public_key";
        clientConfig[config_key::psk_key] = "test_psk_key";
        clientConfig[config_key::client_ip] = "10.8.1.2";
        clientConfig[config_key::allowed_ips] = QJsonArray::fromStringList({"0.0.0.0/0"});

        QJsonObject awgConfig;
        awgConfig[config_key::last_config] = QString(QJsonDocument(clientConfig).toJson());
        awgConfig[config_key::port] = protocols::awg::defaultPort;
        awgConfig[config_key::transport_proto] = "udp";
        awgConfig[config_key::protocolVersion] = protocols::awg::awgV2;
        awgConfig[config_key::subnet_address] = protocols::wireguard::defaultSubnetAddress;
        awgConfig[config_key::junkPacketCount] = protocols::awg::defaultJunkPacketCount;
        awgConfig[config_key::junkPacketMinSize] = protocols::awg::defaultJunkPacketMinSize;
        awgConfig[config_key::junkPacketMaxSize] = protocols::awg::defaultJunkPacketMaxSize;
        awgConfig[config_key::initPacketJunkSize] = protocols::awg::defaultInitPacketJunkSize;
        awgConfig[config_key::responsePacketJunkSize] = protocols::awg::defaultResponsePacketJunkSize;
        awgConfig[config_key::cookieReplyPacketJunkSize] = protocols::awg::defaultCookieReplyPacketJunkSize;
        awgConfig[config_key::transportPacketJunkSize] = protocols::awg::defaultTransportPacketJunkSize;
        awgConfig[config_key::initPacketMagicHeader] = protocols::awg::defaultInitPacketMagicHeader;
        awgConfig[config_key::responsePacketMagicHeader] = protocols::awg::defaultResponsePacketMagicHeader;
        awgConfig[config_key::underloadPacketMagicHeader] = protocols::awg::defaultUnderloadPacketMagicHeader;
        awgConfig[config_key::transportPacketMagicHeader] = protocols::awg::defaultTransportPacketMagicHeader;
        awgConfig[config_key::specialJunk1] = protocols::awg::defaultSpecialJunk1;
        awgConfig[config_key::specialJunk2] = protocols::awg::defaultSpecialJunk2;
        awgConfig[config_key::specialJunk3] = protocols::awg::defaultSpecialJunk3;
        awgConfig[config_key::specialJunk4] = protocols::awg::defaultSpecialJunk4;
        awgConfig[config_key::specialJunk5] = protocols::awg::defaultSpecialJunk5;
        awgConfig[config_key::isThirdPartyConfig] = true;

        QJsonObject container;
        container[config_key::container] = "amnezia-awg";
        container[config_key::awg] = awgConfig;

        QJsonArray containers;
        containers.append(container);

        QJsonObject config;
        config[config_key::containers] = containers;
        config[config_key::defaultContainer] = "amnezia-awg";
        config[config_key::description] = "AWG2 Test Server";
        config[config_key::hostName] = "test.example.com";

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
            m_coreController->m_serversModel->updateModel(QVector<ServerConfig>(), -1, false);
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
            m_coreController->m_serversUiController->setProcessedServerIndex(serverIndex);
            
            ServerConfig serverConfig = m_coreController->m_serversRepository->server(serverIndex);
            QString actualServerName = ServerConfigUtils::description(serverConfig);
            QString containerName = ContainerUtils::containerHumanNames().value(DockerContainer::Awg);
            QString hostName = "test.example.com";
            
            QString collapsedDescription = m_coreController->m_serversUiController->getDefaultServerDescriptionCollapsed();
            QString expectedCollapsed = actualServerName + " " + containerName + " | " + hostName;
            QVERIFY2(collapsedDescription == expectedCollapsed, 
                     QString("Collapsed description should be '%1', got '%2'").arg(expectedCollapsed, collapsedDescription).toUtf8().constData());
            
            QString expandedDescription = m_coreController->m_serversUiController->getDefaultServerDescriptionExpanded();
            QString expectedExpanded = actualServerName + hostName;
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
            QVERIFY2(containerConfig.value(config_key::container).toString() == "amnezia-awg", "Container config should have correct container type");
            
            QJsonObject awgProtocolConfig = containerConfig.value(config_key::awg).toObject();
            QVERIFY2(!awgProtocolConfig.isEmpty(), "AWG protocol config should not be empty");
            
            QString protocolVersion = awgProtocolConfig.value(config_key::protocolVersion).toString();
            QVERIFY2(protocolVersion == protocols::awg::awgV2, QString("Protocol version should be '%1', got '%2'").arg(protocols::awg::awgV2, protocolVersion).toUtf8().constData());
            
            QString port = awgProtocolConfig.value(config_key::port).toString();
            QVERIFY2(port == protocols::awg::defaultPort, QString("Port should be '%1', got '%2'").arg(protocols::awg::defaultPort, port).toUtf8().constData());
            
            QString subnetAddress = awgProtocolConfig.value(config_key::subnet_address).toString();
            QVERIFY2(subnetAddress == protocols::wireguard::defaultSubnetAddress, QString("Subnet address should be '%1', got '%2'").arg(protocols::wireguard::defaultSubnetAddress, subnetAddress).toUtf8().constData());
            
            bool isThirdParty = m_coreController->m_containersModel->data(containerModelIndex, ContainersModel::IsThirdPartyConfigRole).toBool();
            QVERIFY2(isThirdParty == true, "Imported config should be third party config");
            
            DockerContainer dockerContainer = static_cast<DockerContainer>(m_coreController->m_containersModel->data(containerModelIndex, ContainersModel::DockerContainerRole).toInt());
            QVERIFY2(dockerContainer == DockerContainer::Awg, "Docker container should be Awg");
            
            QString containerString = m_coreController->m_containersModel->data(containerModelIndex, ContainersModel::ContainerStringRole).toString();
            QVERIFY2(containerString == "amnezia-awg", "Container string should be amnezia-awg");
        }
    }
};

QTEST_MAIN(TestUiServersModelAndController)
#include "testUiServersModelAndController.moc"
