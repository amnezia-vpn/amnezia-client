#include "vpnConfigirationController.h"

#include "configurators/vpn_configurator.h"

VpnConfigirationsController::VpnConfigirationsController(const std::shared_ptr<Settings> &settings, QObject *parent)
    : QObject { parent }, m_settings(settings)
{
}

ErrorCode VpnConfigirationsController::createProtocolConfigForContainer(const ServerCredentials &credentials,
                                                                        const DockerContainer container,
                                                                        QJsonObject &containerConfig, QString &clientId)
{
    ErrorCode errorCode = ErrorCode::NoError;

    for (Proto protocol : ContainerProps::protocolsForContainer(container)) {
        QJsonObject protocolConfig = containerConfig.value(ProtocolProps::protoToString(protocol)).toObject();

        VpnConfigurator vpnConfigurator(m_settings);
        QString protocolConfigString = vpnConfigurator.genVpnProtocolConfig(credentials, container, containerConfig,
                                                                            protocol, clientId, errorCode);
        if (errorCode != ErrorCode::NoError) {
            return errorCode;
        }

        protocolConfig.insert(config_key::last_config, protocolConfigString);
        containerConfig.insert(ProtocolProps::protoToString(protocol), protocolConfig);
    }

    return errorCode;
}

ErrorCode VpnConfigirationsController::createProtocolConfigString(const int serverIndex,
                                                                  const ServerCredentials &credentials,
                                                                  const DockerContainer container,
                                                                  const QJsonObject &containerConfig, QString &clientId, QString &protocolConfigString)
{
    ErrorCode errorCode = ErrorCode::NoError;
    VpnConfigurator vpnConfigurator(m_settings);
    auto mainProtocol = ContainerProps::defaultProtocol(container);

    protocolConfigString = vpnConfigurator.genVpnProtocolConfig(credentials, container, containerConfig, mainProtocol, clientId, errorCode);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }
    protocolConfigString = vpnConfigurator.processConfigWithExportSettings(serverIndex, container, mainProtocol, protocolConfigString);

    return errorCode;
}

//QMap<Proto, QString> VpnConnection::getLastVpnConfig(const QJsonObject &containerConfig)
//{
//    QMap<Proto, QString> configs;
//    for (Proto proto : ProtocolProps::allProtocols()) {

//        QString cfg = containerConfig.value(ProtocolProps::protoToString(proto))
//                              .toObject()
//                              .value(config_key::last_config)
//                              .toString();

//        if (!cfg.isEmpty())
//            configs.insert(proto, cfg);
//    }
//    return configs;
//}

//QString VpnConnection::createVpnConfigurationForProto(int serverIndex, const ServerCredentials &credentials,
//                                                      DockerContainer container, const QJsonObject &containerConfig,
//                                                      Proto proto, ErrorCode errorCode)
//{
//    QMap<Proto, QString> lastVpnConfig = getLastVpnConfig(containerConfig);

//    QString configData;
//    if (lastVpnConfig.contains(proto)) {
//        configData = lastVpnConfig.value(proto);
//        configData = m_configurator->processConfigWithLocalSettings(serverIndex, container, proto, configData);
//    } else {
//        QString clientId;
//        configData = m_configurator->genVpnProtocolConfig(credentials, container, containerConfig, proto, clientId, errorCode);

//        if (errorCode != ErrorCode::NoError) {
//            return "";
//        }

//        QString configDataBeforeLocalProcessing = configData;

//        configData = m_configurator->processConfigWithLocalSettings(serverIndex, container, proto, configData);

//        if (serverIndex >= 0) {
//            qDebug() << "VpnConnection::createVpnConfiguration: saving config for server #" << serverIndex << container
//                     << proto;
//            QJsonObject protoObject = m_settings->protocolConfig(serverIndex, container, proto);
//            protoObject.insert(config_key::last_config, configDataBeforeLocalProcessing);
//            m_settings->setProtocolConfig(serverIndex, container, proto, protoObject);
//        }

//        if ((container != DockerContainer::Cloak && container != DockerContainer::ShadowSocks) ||
//            ((container == DockerContainer::Cloak || container == DockerContainer::ShadowSocks) && proto == Proto::OpenVpn)) {
//            QEventLoop wait;
//            emit m_configurator->newVpnConfigCreated(clientId, QString("Admin [%1]").arg(QSysInfo::prettyProductName()), container, credentials);
//            QObject::connect(m_configurator.get(), &VpnConfigurator::clientModelUpdated, &wait, &QEventLoop::quit);
//            wait.exec();
//        }
//    }

//    return configData;
//}

//QJsonObject VpnConfigirationsController::createVpnConfiguration(const QJsonObject &serverConfig, const QJsonObject &containerConfig,
//                                                                const DockerContainer container, ErrorCode errorCode)
//{
//    QJsonObject vpnConfiguration;
//    VpnConfigurator vpnConfigurator(m_settings);

//    for (ProtocolEnumNS::Proto proto : ContainerProps::protocolsForContainer(container)) {
//        if (serverConfig.value(config_key::configVersion).toInt() &&
//            container == DockerContainer::Cloak && proto == ProtocolEnumNS::Proto::ShadowSocks) {
//            continue;
//        }

//        QString protocolConfigString = containerConfig.value(ProtocolProps::protoToString(proto)).toObject().value(config_key::last_config).toString();
//        protocolConfigString = vpnConfigurator.processConfigWithLocalSettings(serverIndex, container, proto, protocolConfigString);

//        QJsonObject vpnConfigData =
//                QJsonDocument::fromJson(createVpnConfigurationForProto(serverIndex, credentials, container,
//                                                                       containerConfig, proto, errorCode).toUtf8()).object();

//        if (errorCode != ErrorCode::NoError) {
//            return {};
//        }

//        vpnConfiguration.insert(ProtocolProps::key_proto_config_data(proto), vpnConfigData);
//    }

//    Proto proto = ContainerProps::defaultProtocol(container);
//    vpnConfiguration[config_key::vpnproto] = ProtocolProps::protoToString(proto);

//    auto dns = m_configurator->getDnsForConfig(serverIndex);

//    vpnConfiguration[config_key::dns1] = dns.first;
//    vpnConfiguration[config_key::dns2] = dns.second;

//    const QJsonObject &server = m_settings->server(serverIndex);
//    vpnConfiguration[config_key::hostName] = server.value(config_key::hostName).toString();
//    vpnConfiguration[config_key::description] = server.value(config_key::description).toString();

//    vpnConfiguration[config_key::configVersion] = server.value(config_key::configVersion).toInt();
//    // TODO: try to get hostName, port, description for 3rd party configs
//    // vpnConfiguration[config_key::port] = ...;

//    return vpnConfiguration;
//}

//ErrorCode VpnConfigirationsController::createVpnConfiguration()
//{

//}
