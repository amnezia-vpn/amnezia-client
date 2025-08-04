#include "shadowsocks_configurator.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

#include "core/models/containers/containers_defs.h"
#include "core/controllers/selfhosted/serverController.h"
#include "core/models/protocols/shadowsocksProtocolConfig.h"
#include "protocols/protocols_defs.h"

ShadowSocksConfigurator::ShadowSocksConfigurator(std::shared_ptr<Settings> settings, const QSharedPointer<ServerController> &serverController,
                                                 QObject *parent)
    : ConfiguratorBase(settings, serverController, parent)
{
}

ConfiguratorBase::Vars ShadowSocksConfigurator::generateProtocolVars(const ServerCredentials &credentials, DockerContainer container,
                                                                     const QSharedPointer<ProtocolConfig> &protocolConfig) const
{
    Vars vars = generateCommonVars(credentials, container);

    auto shadowsocksConfig = qSharedPointerCast<ShadowsocksProtocolConfig>(protocolConfig);
    if (!shadowsocksConfig) {
        return vars;
    }

    vars.append({{"$SHADOWSOCKS_SERVER_PORT", shadowsocksConfig->serverProtocolConfig.port}});
    vars.append({{"$SHADOWSOCKS_LOCAL_PORT", protocols::shadowsocks::defaultLocalProxyPort}});
    vars.append({{"$SHADOWSOCKS_CIPHER", shadowsocksConfig->serverProtocolConfig.cipher}});

    return vars;
}

QSharedPointer<ProtocolConfig> ShadowSocksConfigurator::createConfig(const ServerCredentials &credentials, DockerContainer container,
                                                                  const QSharedPointer<ProtocolConfig> &protocolConfig, ErrorCode &errorCode)
{
    auto shadowsocksConfig = qSharedPointerCast<ShadowsocksProtocolConfig>(protocolConfig);
    if (!shadowsocksConfig) {
        errorCode = ErrorCode::InternalError;
        return nullptr;
    }

    QString ssKey =
            m_serverController->getTextFileFromContainer(container, credentials, amnezia::protocols::shadowsocks::ssKeyPath, errorCode);
    ssKey.replace("\n", "");

    if (errorCode != ErrorCode::NoError) {
        return nullptr;
    }

    shadowsocksConfig->clientProtocolConfig.server = credentials.hostName;
    shadowsocksConfig->clientProtocolConfig.serverPort = shadowsocksConfig->serverProtocolConfig.port;
    shadowsocksConfig->clientProtocolConfig.localPort = protocols::shadowsocks::defaultLocalProxyPort;
    shadowsocksConfig->clientProtocolConfig.password = ssKey;
    shadowsocksConfig->clientProtocolConfig.timeout = 60;
    shadowsocksConfig->clientProtocolConfig.method = shadowsocksConfig->serverProtocolConfig.cipher;

    QJsonObject config;
    config.insert("server", shadowsocksConfig->clientProtocolConfig.server);
    config.insert("server_port", shadowsocksConfig->clientProtocolConfig.serverPort);
    config.insert("local_port", shadowsocksConfig->clientProtocolConfig.localPort);
    config.insert("password", shadowsocksConfig->clientProtocolConfig.password);
    config.insert("timeout", shadowsocksConfig->clientProtocolConfig.timeout);
    config.insert("method", shadowsocksConfig->clientProtocolConfig.method);

    QString textCfg = QJsonDocument(config).toJson();

    shadowsocksConfig->clientProtocolConfig.isEmpty = false;
    shadowsocksConfig->clientProtocolConfig.nativeConfig = textCfg;

    return shadowsocksConfig;
}
