#include "cloakConfigurator.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

#include "containers/containers_defs.h"
#include "core/utils/selfhosted/sshSession.h"
#include "core/utils/selfhosted/scriptsRegistry.h"
#include "protocols/protocols_defs.h"

CloakConfigurator::CloakConfigurator(std::shared_ptr<Settings> settings, SshSession* sshSession, QObject *parent)
    : ConfiguratorBase(settings, sshSession, parent)
{
}

QString CloakConfigurator::createConfig(const ServerCredentials &credentials, DockerContainer container, const QJsonObject &containerConfig,
                                        ErrorCode &errorCode)
{
    QString cloakPublicKey =
            m_sshSession->getTextFileFromContainer(container, credentials, amnezia::protocols::cloak::ckPublicKeyPath, errorCode);
    cloakPublicKey.replace("\n", "");

    if (errorCode != ErrorCode::NoError) {
        return "";
    }

    QString cloakBypassUid =
            m_sshSession->getTextFileFromContainer(container, credentials, amnezia::protocols::cloak::ckBypassUidKeyPath, errorCode);
    cloakBypassUid.replace("\n", "");

    if (errorCode != ErrorCode::NoError) {
        return "";
    }

    QJsonObject config;
    config.insert("Transport", "direct");
    config.insert("ProxyMethod", "openvpn");
    config.insert("EncryptionMethod", "aes-gcm");
    config.insert("UID", cloakBypassUid);
    config.insert("PublicKey", cloakPublicKey);
    config.insert("ServerName", "$FAKE_WEB_SITE_ADDRESS");
    config.insert("NumConn", 1);
    config.insert("BrowserSig", "chrome");
    config.insert("StreamTimeout", 300);
    config.insert("RemoteHost", credentials.hostName);
    config.insert("RemotePort", "$CLOAK_SERVER_PORT");

    amnezia::ScriptVars vars = amnezia::genBaseVars(credentials, container, m_settings->primaryDns(), m_settings->secondaryDns());
    vars.append(amnezia::genProtocolVarsForContainer(container, containerConfig));
    QString textCfg = m_sshSession->replaceVars(QJsonDocument(config).toJson(), vars);

    return textCfg;
}
