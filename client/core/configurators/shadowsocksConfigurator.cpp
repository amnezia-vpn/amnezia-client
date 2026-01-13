#include "shadowsocks_configurator.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

#include "containers/containers_defs.h"
#include "core/utils/selfhosted/sshSession.h"
#include "core/utils/selfhosted/scriptsRegistry.h"
#include "protocols/protocols_defs.h"

ShadowSocksConfigurator::ShadowSocksConfigurator(std::shared_ptr<Settings> settings, SshSession* sshSession,
                                                 QObject *parent)
    : ConfiguratorBase(settings, sshSession, parent)
{
}

QString ShadowSocksConfigurator::createConfig(const ServerCredentials &credentials, DockerContainer container,
                                              const QJsonObject &containerConfig, ErrorCode &errorCode)
{
    QString ssKey =
            m_sshSession->getTextFileFromContainer(container, credentials, amnezia::protocols::shadowsocks::ssKeyPath, errorCode);
    ssKey.replace("\n", "");

    if (errorCode != ErrorCode::NoError) {
        return "";
    }

    QJsonObject config;
    config.insert("server", credentials.hostName);
    config.insert("server_port", "$SHADOWSOCKS_SERVER_PORT");
    config.insert("local_port", "$SHADOWSOCKS_LOCAL_PORT");
    config.insert("password", ssKey);
    config.insert("timeout", 60);
    config.insert("method", "$SHADOWSOCKS_CIPHER");

    amnezia::ScriptVars vars = amnezia::genBaseVars(credentials, container, m_settings->primaryDns(), m_settings->secondaryDns());
    vars.append(amnezia::genProtocolVarsForContainer(container, containerConfig));
    QString textCfg = m_sshSession->replaceVars(QJsonDocument(config).toJson(), vars);

    // qDebug().noquote() << textCfg;
    return textCfg;
}
