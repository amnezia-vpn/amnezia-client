#include "shadowsocks_configurator.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

#include "containers/containers_defs.h"
#include "core/controllers/serverController.h"
#include "core/models/protocols/shadowsocksProtocolConfig.h"

ShadowSocksConfigurator::ShadowSocksConfigurator(std::shared_ptr<Settings> settings,
                                                 const QSharedPointer<ServerController> &serverController, QObject *parent)
    : ConfiguratorBase(settings, serverController, parent)
{
}

QSharedPointer<ProtocolConfig> ShadowSocksConfigurator::createConfig(const ServerCredentials &serverCredentials,
                                                                     const ContainerConfig &containerConfig, ErrorCode &errorCode)
{
    QString ssKey = m_serverController->getTextFileFromContainer(containerConfig.containerType, serverCredentials,
                                                                 amnezia::protocols::shadowsocks::ssKeyPath, errorCode);
    ssKey.replace("\n", "");

    if (errorCode != ErrorCode::NoError) {
        return nullptr;
    }

    QJsonObject config;
    config.insert("server", serverCredentials.hostName);
    config.insert("server_port", "$SHADOWSOCKS_SERVER_PORT");
    config.insert("local_port", protocols::shadowsocks::defaultLocalProxyPort);
    config.insert("password", ssKey);
    config.insert("timeout", 60);
    config.insert("method", "$SHADOWSOCKS_CIPHER");

    QString textCfg = m_serverController->replaceVars(QJsonDocument(config).toJson(),
                                                      m_serverController->genVarsForScript(containerConfig, serverCredentials));

    auto baseProtocolConfig = qSharedPointerCast<ShadowsocksProtocolConfig>(
            containerConfig.protocolConfigs.value(ProtocolProps::protoToString(Proto::ShadowSocks)));
    auto protocolConfig = QSharedPointer<ShadowsocksProtocolConfig>::create(*baseProtocolConfig);

    protocolConfig->clientProtocolConfig.isEmpty = false;
    protocolConfig->clientProtocolConfig.nativeConfig = textCfg;

    return protocolConfig;
}
