#include "xrayConfigurator.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QThread>
#include <QUuid>
#include "logger.h"

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/utils/selfhosted/sshSession.h"
#include "core/utils/selfhosted/scriptsRegistry.h"
#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/models/containerConfig.h"
#include "core/models/protocols/xrayProtocolConfig.h"

namespace {
    Logger logger("XrayConfigurator");

    QString normalizeXhttpMode(const QString &m) { return amnezia::xrayEffective::xhttpMode(m); }
    QString normalizeSessionSeqPlacement(const QString &p) { return amnezia::xrayEffective::sessionSeqPlacement(p); }
    QString normalizeUplinkDataPlacement(const QString &p) { return amnezia::xrayEffective::uplinkDataPlacement(p); }
    QString normalizeXPaddingPlacement(const QString &p) { return amnezia::xrayEffective::xPaddingPlacement(p); }
    QString normalizeXPaddingMethod(const QString &m) { return amnezia::xrayEffective::xPaddingMethod(m); }

    QString makeRangeString(const QString &minV, const QString &maxV)
    {
        return amnezia::xrayEffective::range(minV, maxV);
    }

    void putIntRangeIfAny(QJsonObject &obj, const char *key, QString minV, QString maxV, const char *fallbackMin,
                          const char *fallbackMax)
    {
        amnezia::xrayEffective::putRangeIfAny(obj, key, minV, maxV, fallbackMin, fallbackMax);
    }

    QString effectiveClientFlow(const amnezia::XrayServerConfig &srv)
    {
        return amnezia::xrayEffective::clientFlow(srv);
    }

    QString effectiveSecurity(const amnezia::XrayServerConfig &srv)
    {
        return amnezia::xrayEffective::security(srv);
    }

    // Desktop applies this in XrayProtocol::start(); iOS/Android pass JSON straight to libxray — same fixes here.
    void sanitizeXrayNativeConfig(amnezia::ProtocolConfig &pc)
    {
        QString c = pc.nativeConfig();
        if (c.isEmpty()) {
            return;
        }
        bool changed = false;
        if (c.contains(QLatin1String("Mozilla/5.0"), Qt::CaseInsensitive)) {
            c.replace(QLatin1String("Mozilla/5.0"), QString::fromLatin1(amnezia::protocols::xray::defaultFingerprint),
                      Qt::CaseInsensitive);
            changed = true;
        }
        const QString legacyListen = QString::fromLatin1(amnezia::protocols::xray::defaultLocalAddr);
        const QString listenOk = QString::fromLatin1(amnezia::protocols::xray::defaultLocalListenAddr);
        if (c.contains(legacyListen)) {
            c.replace(legacyListen, listenOk);
            changed = true;
        }
        if (changed) {
            pc.setNativeConfig(c);
        }
    }
} // namespace

XrayConfigurator::XrayConfigurator(SshSession* sshSession, QObject *parent)
    : ConfiguratorBase(sshSession, parent)
{
}

amnezia::ProtocolConfig XrayConfigurator::processConfigWithLocalSettings(const amnezia::ConnectionSettings &settings,
                                                                         amnezia::ProtocolConfig protocolConfig)
{
    applyDnsToNativeConfig(settings.dns, protocolConfig);
    sanitizeXrayNativeConfig(protocolConfig);
    return protocolConfig;
}

ErrorCode XrayConfigurator::uploadServerConfigJson(const ServerCredentials &credentials, DockerContainer container,
                                                    const DnsSettings &dnsSettings, const QJsonObject &serverConfig) const
{
    const QString updatedConfig = QJsonDocument(serverConfig).toJson();
    ErrorCode errorCode = m_sshSession->uploadTextFileToContainer(
            container, credentials, updatedConfig, amnezia::protocols::xray::serverConfigPath,
            libssh::ScpOverwriteMode::ScpOverwriteExisting);
    if (errorCode != ErrorCode::NoError) {
        logger.error() << "Failed to upload updated config";
        return errorCode;
    }

    const QString restartScript = QStringLiteral("sudo docker restart $CONTAINER_NAME");
    errorCode = m_sshSession->runScript(
            credentials,
            m_sshSession->replaceVars(restartScript,
                                      amnezia::genBaseVars(credentials, container, dnsSettings.primaryDns,
                                                           dnsSettings.secondaryDns)));
    if (errorCode != ErrorCode::NoError) {
        logger.error() << "Failed to restart container";
    }
    return errorCode;
}

ErrorCode XrayConfigurator::readRealityKeyFiles(const DockerContainer container, const ServerCredentials &credentials,
                                                QString &outPublicKey, QString &outShortId) const
{
    outPublicKey.clear();
    outShortId.clear();

    auto readKeyFile = [&](const QString &path, QString &out) -> ErrorCode {
        for (int attempt = 0; attempt < 3; ++attempt) {
            ErrorCode fileError = ErrorCode::NoError;
            out = QString::fromUtf8(m_sshSession->getTextFileFromContainer(container, credentials, path, fileError));
            out.replace(QLatin1Char('\n'), QString());
            out.replace(QLatin1Char('\r'), QString());
            if (fileError == ErrorCode::NoError && !out.isEmpty()) {
                return ErrorCode::NoError;
            }
            if (attempt < 2) {
                QThread::msleep(500);
            }
        }
        logger.error() << "Xray readRealityKeyFiles: failed path=" << path;
        return ErrorCode::XrayRealityKeysReadFailed;
    };

    ErrorCode errorCode = readKeyFile(QString::fromLatin1(amnezia::protocols::xray::PublicKeyPath), outPublicKey);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }
    return readKeyFile(QString::fromLatin1(amnezia::protocols::xray::shortidPath), outShortId);
}


ErrorCode XrayConfigurator::applyServerSettingsToRemote(const ServerCredentials &credentials, DockerContainer container,
                                                        ContainerConfig &containerConfig, const DnsSettings &dnsSettings,
                                                        bool appendNewClient, QString *outClientId)
{
    ErrorCode errorCode = ErrorCode::NoError;
    const auto *xrayCfg = containerConfig.protocolConfig.as<XrayProtocolConfig>();
    if (!xrayCfg) {
        logger.error() << "Xray applyServerSettings: missing XrayProtocolConfig";
        return ErrorCode::InternalError;
    }

    const XrayServerConfig &srv = xrayCfg->serverConfig;
    if (srv.isThirdPartyConfig) {
        if (outClientId && xrayCfg->hasClientConfig()) {
            *outClientId = xrayCfg->clientConfig->id;
        }
        return ErrorCode::NoError;
    }

    const QString flowValue = effectiveClientFlow(srv);
    QString realityPublicKey;
    QString realityShortId;
    if (effectiveSecurity(srv) == QLatin1String("reality")) {
        errorCode = readRealityKeyFiles(container, credentials, realityPublicKey, realityShortId);
        if (errorCode != ErrorCode::NoError) {
            logger.error() << "Xray applyServerSettings: readRealityKeyFiles failed, error="
                           << static_cast<int>(errorCode);
            return errorCode;
        }
    }

    QString currentConfig = m_sshSession->getTextFileFromContainer(
            container, credentials, amnezia::protocols::xray::serverConfigPath, errorCode);
    if (errorCode != ErrorCode::NoError) {
        logger.error() << "Xray applyServerSettings: getTextFileFromContainer failed, error="
                       << static_cast<int>(errorCode) << "path=" << amnezia::protocols::xray::serverConfigPath;
        return errorCode;
    }

    QJsonDocument doc = QJsonDocument::fromJson(currentConfig.toUtf8());
    if (doc.isNull() || !doc.isObject()) {
        logger.error() << "Failed to parse server config JSON";
        return ErrorCode::XrayServerConfigInvalid;
    }

    QJsonObject serverConfig = doc.object();
    const QJsonObject originalConfig = serverConfig;
    if (!serverConfig.contains(amnezia::protocols::xray::inbounds)) {
        logger.error() << "Server config missing 'inbounds' field";
        return ErrorCode::XrayServerConfigInvalid;
    }

    QJsonArray inbounds = serverConfig[amnezia::protocols::xray::inbounds].toArray();
    if (inbounds.isEmpty()) {
        logger.error() << "Server config has empty 'inbounds' array";
        return ErrorCode::XrayServerConfigInvalid;
    }

    QJsonObject inbound = inbounds[0].toObject();
    if (!inbound.contains(amnezia::protocols::xray::settings)) {
        logger.error() << "Inbound missing 'settings' field";
        return ErrorCode::XrayServerConfigInvalid;
    }

    QJsonObject settings = inbound[amnezia::protocols::xray::settings].toObject();
    if (!settings.contains(amnezia::protocols::xray::clients)) {
        settings[amnezia::protocols::xray::clients] = QJsonArray {};
    }

    QJsonArray clients = settings[amnezia::protocols::xray::clients].toArray();
    QString clientId;

    if (appendNewClient) {
        const bool adoptingOwnAccount = !(xrayCfg->hasClientConfig() && !xrayCfg->clientConfig->id.isEmpty());

        QString ownClientId;
        if (readContainerKeyFile(container, credentials, QString::fromLatin1(amnezia::protocols::xray::uuidPath),
                                 ownClientId)
            != ErrorCode::NoError) {
            ownClientId.clear();
        }

        int existingIndex = -1;
        if (adoptingOwnAccount && !ownClientId.isEmpty()) {
            for (int i = 0; i < clients.size(); ++i) {
                if (clients[i].toObject().value(amnezia::protocols::xray::id).toString() == ownClientId) {
                    existingIndex = i;
                    break;
                }
            }
        }

        if (existingIndex >= 0) {
            clientId = ownClientId;
            QJsonObject clientEntry = clients[existingIndex].toObject();
            if (flowValue.isEmpty()) {
                clientEntry.remove(amnezia::protocols::xray::flow);
            } else {
                clientEntry[amnezia::protocols::xray::flow] = flowValue;
            }
            clients[existingIndex] = clientEntry;
            logger.info() << "Xray applyServerSettings: reusing the account from the key file, clients="
                          << clients.size();
        } else {
            const bool takeOverOwnAccount = adoptingOwnAccount && !ownClientId.isEmpty();
            clientId = takeOverOwnAccount ? ownClientId : QUuid::createUuid().toString(QUuid::WithoutBraces);
            QJsonObject clientEntry;
            clientEntry[amnezia::protocols::xray::id] = clientId;
            if (!flowValue.isEmpty()) {
                clientEntry[amnezia::protocols::xray::flow] = flowValue;
            }
            clients.append(clientEntry);
            logger.info() << "Xray applyServerSettings: added an account,"
                          << (takeOverOwnAccount ? "taken over from the key file" : "freshly minted for a new user")
                          << ", clients=" << clients.size();
        }
    } else {
        if (clients.isEmpty()) {
            logger.error() << "Server config has no VLESS clients";
            return ErrorCode::XrayServerNoVlessClients;
        }
        clientId = clients[0].toObject()[amnezia::protocols::xray::id].toString();
        if (clientId.isEmpty()) {
            logger.error() << "Server config VLESS client has empty id";
            return ErrorCode::XrayServerNoVlessClients;
        }
        QJsonArray updatedClients;
        for (const QJsonValue &v : clients) {
            QJsonObject c = v.toObject();
            if (flowValue.isEmpty()) {
                c.remove(amnezia::protocols::xray::flow);
            } else {
                c[amnezia::protocols::xray::flow] = flowValue;
            }
            updatedClients.append(c);
        }
        clients = updatedClients;
    }

    settings[amnezia::protocols::xray::clients] = clients;
    inbound[amnezia::protocols::xray::settings] = settings;
    inbounds[0] = inbound;
    serverConfig[amnezia::protocols::xray::inbounds] = inbounds;

    const QString listenPort = srv.port.isEmpty() ? QString::fromLatin1(amnezia::protocols::xray::defaultPort)
                                                  : srv.port;
    if (serverConfig == originalConfig) {
        logger.info() << "Xray applyServerSettings: server config already matches, no rewrite and no restart,"
                      << "clients=" << clients.size();
    } else {
        errorCode = uploadServerConfigAtomically(credentials, container, listenPort, serverConfig);
        if (errorCode != ErrorCode::NoError) {
            logger.error() << "Xray applyServerSettings: apply failed, error=" << static_cast<int>(errorCode);
            return errorCode;
        }
    }

    if (outClientId) {
        *outClientId = clientId;
    }

    XrayProtocolConfig updated =
            buildClientProtocolConfig(credentials, container, srv, xrayCfg->clientTemplate, clientId, errorCode,
                                      realityPublicKey, realityShortId);
    if (errorCode != ErrorCode::NoError) {
        logger.error() << "Xray applyServerSettings: buildClientProtocolConfig failed, error="
                       << static_cast<int>(errorCode);
        return errorCode;
    }
    containerConfig.protocolConfig = updated;
    return ErrorCode::NoError;
}

ErrorCode XrayConfigurator::readContainerKeyFile(DockerContainer container, const ServerCredentials &credentials,
                                                 const QString &path, QString &out) const
{
    out.clear();
    for (int attempt = 0; attempt < 3; ++attempt) {
        ErrorCode fileError = ErrorCode::NoError;
        out = QString::fromUtf8(m_sshSession->getTextFileFromContainer(container, credentials, path, fileError));
        out.replace(QLatin1Char('\n'), QString());
        out.replace(QLatin1Char('\r'), QString());
        if (fileError == ErrorCode::NoError && !out.isEmpty()) {
            return ErrorCode::NoError;
        }
        if (attempt < 2) {
            QThread::msleep(500);
        }
    }
    logger.error() << "Xray readContainerKeyFile: failed path=" << path;
    return ErrorCode::XrayRealityKeysReadFailed;
}

ErrorCode XrayConfigurator::writeServerConfigForSetup(const ServerCredentials &credentials, DockerContainer container,
                                                      ContainerConfig &containerConfig, const DnsSettings &dnsSettings)
{
    Q_UNUSED(dnsSettings);
    namespace px = amnezia::protocols::xray;

    const auto *xrayCfg = containerConfig.protocolConfig.as<XrayProtocolConfig>();
    if (!xrayCfg) {
        logger.error() << "Xray writeServerConfigForSetup: missing XrayProtocolConfig";
        return ErrorCode::InternalError;
    }
    const XrayServerConfig &srv = xrayCfg->serverConfig;
    if (srv.isThirdPartyConfig) {
        return ErrorCode::NoError;
    }

    if (!isSecuritySupportedOnSelfHosted(srv)) {
        logger.error() << "Xray writeServerConfigForSetup: refused, security is not supported for self-hosted, security="
                       << srv.security << "transport=" << srv.transport;
        return ErrorCode::XrayTlsNotSupported;
    }

    ErrorCode errorCode = ErrorCode::NoError;

    QString clientId;
    errorCode = readContainerKeyFile(container, credentials, QString::fromLatin1(px::uuidPath), clientId);
    if (errorCode != ErrorCode::NoError) {
        logger.error() << "Xray writeServerConfigForSetup: could not read the container uuid, error="
                       << static_cast<int>(errorCode);
        return errorCode;
    }

    const QString securityEff = effectiveSecurity(srv);

    QString realityPrivateKey;
    QString realityPublicKey;
    QString realityShortId;
    if (securityEff == QLatin1String("reality")) {
        errorCode = readContainerKeyFile(container, credentials, QString::fromLatin1(px::PrivateKeyPath), realityPrivateKey);
        if (errorCode != ErrorCode::NoError)
            return errorCode;
        errorCode = readContainerKeyFile(container, credentials, QString::fromLatin1(px::PublicKeyPath), realityPublicKey);
        if (errorCode != ErrorCode::NoError)
            return errorCode;
        errorCode = readContainerKeyFile(container, credentials, QString::fromLatin1(px::shortidPath), realityShortId);
        if (errorCode != ErrorCode::NoError)
            return errorCode;
    }

    const QString flowValue = effectiveClientFlow(srv);
    const QJsonArray clients = collectServerClients(credentials, container, flowValue, clientId, errorCode);
    if (errorCode != ErrorCode::NoError) {
        logger.error() << "Xray writeServerConfigForSetup: refusing to write over an unreadable client list";
        return errorCode;
    }

    const QString portEff = srv.port.isEmpty() ? QString::fromLatin1(px::defaultPort) : srv.port;

    const QJsonObject serverConfig =
            buildServerConfigJson(srv, clients, clientId, realityPrivateKey, realityShortId);


    const QString json = QString::fromUtf8(QJsonDocument(serverConfig).toJson());
    errorCode = m_sshSession->uploadTextFileToContainer(container, credentials, json,
                                                        QString::fromLatin1(px::serverConfigPath),
                                                        libssh::ScpOverwriteMode::ScpOverwriteExisting);
    if (errorCode != ErrorCode::NoError) {
        logger.error() << "Xray writeServerConfigForSetup: upload failed, error=" << static_cast<int>(errorCode)
                       << "port=" << portEff;
        return errorCode;
    }

    XrayProtocolConfig updated =
            buildClientProtocolConfig(credentials, container, srv, xrayCfg->clientTemplate, clientId, errorCode,
                                      realityPublicKey, realityShortId);
    if (errorCode != ErrorCode::NoError) {
        logger.error() << "Xray writeServerConfigForSetup: buildClientProtocolConfig failed, error="
                       << static_cast<int>(errorCode);
        return errorCode;
    }
    updated.clientTemplate.serverFingerprint = srv.sharedBlockFingerprint();
    updated.clientTemplate.updatedAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    updated.clientTemplate.pendingServerUpload =
            !uploadClientTemplate(credentials, container, updated.clientTemplate);
    containerConfig.protocolConfig = updated;
    return ErrorCode::NoError;
}

QJsonArray XrayConfigurator::collectServerClients(const ServerCredentials &credentials, DockerContainer container,
                                                   const QString &flowValue, const QString &fallbackClientId,
                                                   ErrorCode &outError) const
{
    namespace px = amnezia::protocols::xray;

    outError = ErrorCode::NoError;
    const QString configPath = QString::fromLatin1(px::serverConfigPath);

    QString currentConfig;
    for (int attempt = 0; attempt < 3; ++attempt) {
        ErrorCode readError = ErrorCode::NoError;
        currentConfig = m_sshSession->getTextFileFromContainer(container, credentials, configPath, readError);
        if (readError == ErrorCode::NoError && !currentConfig.trimmed().isEmpty()) {
            break;
        }
        currentConfig.clear();
        if (attempt < 2) {
            QThread::msleep(500);
        }
    }

    QJsonArray existing;
    if (currentConfig.trimmed().isEmpty()) {
        QString probeOut;
        auto collect = [&probeOut](const QString &data, libssh::Client &) {
            probeOut += data + "\n";
            return ErrorCode::NoError;
        };
        const QString probe =
                QStringLiteral("if [ -s %1 ]; then echo amnezia_config=present; else echo amnezia_config=absent; fi")
                        .arg(configPath);
        const ErrorCode probeError = m_sshSession->runContainerScript(credentials, container, probe, collect, collect);

        if (probeError != ErrorCode::NoError || !probeOut.contains(QLatin1String("amnezia_config="))) {
            logger.error() << "Xray collectServerClients: read outcome=unknown, cannot tell whether a server config "
                              "exists, probeError="
                           << static_cast<int>(probeError);
            outError = ErrorCode::XrayServerConfigInvalid;
            return {};
        }
        if (probeOut.contains(QLatin1String("amnezia_config=present"))) {
            logger.error() << "Xray collectServerClients: read outcome=unreadable, server config exists but could not "
                              "be read, aborting so the existing client list is not lost";
            outError = ErrorCode::XrayServerConfigInvalid;
            return {};
        }
    } else {
        const QJsonDocument doc = QJsonDocument::fromJson(currentConfig.toUtf8());
        if (!doc.isObject()) {
            logger.error() << "Xray collectServerClients: read outcome=unparseable, server config is not valid JSON, "
                              "bytes="
                           << currentConfig.size();
            outError = ErrorCode::XrayServerConfigInvalid;
            return {};
        }
        const QJsonArray inbounds = doc.object().value(px::inbounds).toArray();
        if (!inbounds.isEmpty()) {
            existing = inbounds[0].toObject().value(px::settings).toObject().value(px::clients).toArray();
        }
        logger.info() << "Xray collectServerClients: read outcome=ok, bytes=" << currentConfig.size()
                      << "inbounds=" << inbounds.size() << "existingClients=" << existing.size();
    }

    QJsonArray clients;
    for (const QJsonValue &value : existing) {
        QJsonObject client = value.toObject();
        if (client.value(px::id).toString().isEmpty()) {
            continue;
        }
        if (flowValue.isEmpty()) {
            client.remove(px::flow);
        } else {
            client[px::flow] = flowValue;
        }
        clients.append(client);
    }

    if (clients.isEmpty() && !fallbackClientId.isEmpty()) {
        QJsonObject fallback;
        fallback[px::id] = fallbackClientId;
        if (!flowValue.isEmpty()) {
            fallback[px::flow] = flowValue;
        }
        clients.append(fallback);
        logger.info() << "Xray collectServerClients: fallbackSeed=used, no client list on server, seeding one client "
                         "from the container uuid, flow="
                      << (flowValue.isEmpty() ? "none" : "set");
    } else {
        logger.info() << "Xray collectServerClients: fallbackSeed=not-used, carrying" << clients.size()
                      << "clients, skippedWithoutId=" << (existing.size() - clients.size())
                      << "flow=" << (flowValue.isEmpty() ? "none" : "set");
    }

    return clients;
}

QJsonObject XrayConfigurator::buildServerConfigJson(const XrayServerConfig &srv, const QJsonArray &clients,
                                                    const QString &streamClientId, const QString &realityPrivateKey,
                                                    const QString &realityShortId) const
{
    namespace px = amnezia::protocols::xray;

    Q_UNUSED(streamClientId);
    const QString securityEff = effectiveSecurity(srv);
    if (securityEff != srv.security) {
        logger.warning() << "Xray buildServerConfigJson: security downgraded for the server config, requested="
                         << srv.security << "written=" << securityEff << "transport=" << srv.transport;
    }
    QJsonObject streamSettings = srv.serverStreamSettings();
    if (securityEff == QLatin1String("reality")) {
        const QString siteEff = srv.site.isEmpty() ? QString::fromLatin1(px::defaultSite) : srv.site;
        const QString sniEff = srv.sni.isEmpty() ? siteEff : srv.sni;
        QJsonObject rs;
        rs[QStringLiteral("dest")] = siteEff + QStringLiteral(":443");
        rs[QStringLiteral("privateKey")] = realityPrivateKey;
        rs[px::serverNames] = QJsonArray { sniEff };
        rs[QStringLiteral("shortIds")] = QJsonArray { realityShortId };
        streamSettings[px::realitySettings] = rs;
    }

    QJsonObject settings;
    settings[px::clients] = clients;
    settings[QStringLiteral("decryption")] = QStringLiteral("none");

    QJsonObject inbound;
    inbound[px::port] = srv.port.isEmpty() ? QString(px::defaultPort).toInt() : srv.port.toInt();
    inbound[QStringLiteral("protocol")] = QStringLiteral("vless");
    inbound[px::settings] = settings;
    inbound[px::streamSettings] = streamSettings;

    QJsonObject serverConfig;
    serverConfig[QStringLiteral("log")] = QJsonObject { { QStringLiteral("loglevel"), QStringLiteral("error") } };
    serverConfig[px::inbounds] = QJsonArray { inbound };
    serverConfig[px::outbounds] =
            QJsonArray { QJsonObject { { QStringLiteral("protocol"), QStringLiteral("freedom") } } };

    return serverConfig;
}

bool XrayConfigurator::isSecuritySupportedOnSelfHosted(const XrayServerConfig &srv)
{
    return effectiveSecurity(srv) != QLatin1String("tls");
}

ErrorCode XrayConfigurator::uploadServerConfigAtomically(const ServerCredentials &credentials,
                                                         DockerContainer container, const QString &listenPort,
                                                         const QJsonObject &serverConfig) const
{
    namespace px = amnezia::protocols::xray;

    const QString livePath = QString::fromLatin1(px::serverConfigPath);
    const QString stagedPath = QStringLiteral("/opt/amnezia/xray/server.new.json");

    const QString json = QString::fromUtf8(QJsonDocument(serverConfig).toJson());
    ErrorCode errorCode = m_sshSession->uploadTextFileToContainer(container, credentials, json, stagedPath,
                                                                  libssh::ScpOverwriteMode::ScpOverwriteExisting);
    if (errorCode != ErrorCode::NoError) {
        logger.error() << "Xray atomic apply: failed to stage config, error=" << static_cast<int>(errorCode);
        return errorCode;
    }

    const QString script = QStringLiteral(
            "LIVE=%1\n"
            "STAGED=%2\n"
            "PORT=%3\n"
            "BACKUP=\"$LIVE.bak\"\n"
            "if [ ! -s \"$STAGED\" ]; then echo \"amnezia_apply=missing\"; exit 0; fi\n"
            "if ! xray -test -format json -config \"$STAGED\" > /tmp/xray_test.log 2>&1; then\n"
            "  echo \"amnezia_apply=invalid\"\n"
            "  cat /tmp/xray_test.log\n"
            "  rm -f \"$STAGED\"\n"
            "  exit 0\n"
            "fi\n"
            "serving() {\n"
            "  netstat -lntup 2>/dev/null | grep -E \"[:.]$PORT[[:space:]]\" | grep -q xray\n"
            "}\n"
            "if [ -f \"$LIVE\" ]; then cp \"$LIVE\" \"$BACKUP\"; fi\n"
            "mv \"$STAGED\" \"$LIVE\"\n"
            "killall -KILL xray 2>/dev/null\n"
            "sleep 1\n"
            "(xray -config \"$LIVE\" > /dev/null 2>&1 &)\n"
            "sleep 3\n"
            "if serving; then\n"
            "  rm -f \"$BACKUP\"\n"
            "  echo \"amnezia_apply=applied\"\n"
            "  exit 0\n"
            "fi\n"
            "if [ -f \"$BACKUP\" ]; then\n"
            "  cp \"$BACKUP\" \"$LIVE\"\n"
            "  killall -KILL xray 2>/dev/null\n"
            "  sleep 1\n"
            "  (xray -config \"$LIVE\" > /dev/null 2>&1 &)\n"
            "  sleep 3\n"
            "  if serving; then echo \"amnezia_apply=rolled_back\"; else echo \"amnezia_apply=rollback_failed\"; fi\n"
            "  exit 0\n"
            "fi\n"
            "echo \"amnezia_apply=rollback_failed\"\n")
                                   .arg(livePath, stagedPath, listenPort);

    QString stdOut;
    auto collect = [&stdOut](const QString &data, libssh::Client &) {
        stdOut += data + "\n";
        return ErrorCode::NoError;
    };

    errorCode = m_sshSession->runContainerScript(credentials, container, script, collect, collect);
    if (errorCode != ErrorCode::NoError) {
        logger.error() << "Xray atomic apply: script failed, error=" << static_cast<int>(errorCode);
        return errorCode;
    }

    if (stdOut.contains(QLatin1String("amnezia_apply=applied"))) {
        logger.info() << "Xray atomic apply: outcome=applied, new config is live and serving on port=" << listenPort;
        return ErrorCode::NoError;
    }
    if (stdOut.contains(QLatin1String("amnezia_apply=missing"))) {
        logger.error() << "Xray atomic apply: outcome=staged-file-missing, staged config was not on the server, "
                          "live config untouched, port="
                       << listenPort;
        return ErrorCode::XrayServerConfigRejected;
    }
    if (stdOut.contains(QLatin1String("amnezia_apply=invalid"))) {
        logger.error() << "Xray atomic apply: outcome=rejected, xray -test refused the config, live config untouched, "
                          "port="
                       << listenPort << "testOutputBytes=" << stdOut.size() << "(output withheld, may contain keys)";
        return ErrorCode::XrayServerConfigRejected;
    }
    if (stdOut.contains(QLatin1String("amnezia_apply=rolled_back"))) {
        logger.error() << "Xray atomic apply: outcome=rolled-back, new config did not serve on port=" << listenPort
                       << ", previous config restored and serving";
        return ErrorCode::XrayServerConfigRolledBack;
    }
    if (stdOut.contains(QLatin1String("amnezia_apply=rollback_failed"))) {
        logger.error() << "Xray atomic apply: outcome=ROLLBACK-FAILED, nothing is serving on port=" << listenPort
                       << ", the xray container is down and the previous config could not be brought back";
        return ErrorCode::XrayServerNotServing;
    }

    logger.error() << "Xray atomic apply: outcome=unrecognised, no marker in script output, port=" << listenPort
                   << "outputBytes=" << stdOut.size() << "output="
                   << stdOut.simplified().left(200);
    return ErrorCode::XrayServerConfigInvalid;
}

ErrorCode XrayConfigurator::applyServerSettingsInPlace(const ServerCredentials &credentials, DockerContainer container,
                                                       ContainerConfig &containerConfig, const DnsSettings &dnsSettings)
{
    Q_UNUSED(dnsSettings);
    namespace px = amnezia::protocols::xray;

    const auto *xrayCfg = containerConfig.protocolConfig.as<XrayProtocolConfig>();
    if (!xrayCfg) {
        logger.error() << "Xray applyServerSettingsInPlace: missing XrayProtocolConfig";
        return ErrorCode::InternalError;
    }

    const XrayServerConfig &srv = xrayCfg->serverConfig;
    if (srv.isThirdPartyConfig) {
        return ErrorCode::NoError;
    }

    if (!isSecuritySupportedOnSelfHosted(srv)) {
        logger.error() << "Xray applyServerSettingsInPlace: refused, security is not supported for self-hosted, security="
                       << srv.security << "transport=" << srv.transport;
        return ErrorCode::XrayTlsNotSupported;
    }


    ErrorCode errorCode = ErrorCode::NoError;

    QString clientId;
    errorCode = readContainerKeyFile(container, credentials, QString::fromLatin1(px::uuidPath), clientId);
    if (errorCode != ErrorCode::NoError) {
        logger.error() << "Xray applyServerSettingsInPlace: could not read the container uuid, error="
                       << static_cast<int>(errorCode);
        return errorCode;
    }

    const QString securityEff = effectiveSecurity(srv);
    QString realityPrivateKey;
    QString realityPublicKey;
    QString realityShortId;
    if (securityEff == QLatin1String("reality")) {
        errorCode = readContainerKeyFile(container, credentials, QString::fromLatin1(px::PrivateKeyPath), realityPrivateKey);
        if (errorCode != ErrorCode::NoError)
            return errorCode;
        errorCode = readContainerKeyFile(container, credentials, QString::fromLatin1(px::PublicKeyPath), realityPublicKey);
        if (errorCode != ErrorCode::NoError)
            return errorCode;
        errorCode = readContainerKeyFile(container, credentials, QString::fromLatin1(px::shortidPath), realityShortId);
        if (errorCode != ErrorCode::NoError)
            return errorCode;
    }

    const QString flowValue = effectiveClientFlow(srv);
    const QJsonArray updatedClients = collectServerClients(credentials, container, flowValue, clientId, errorCode);
    if (errorCode != ErrorCode::NoError) {
        logger.error() << "Xray applyServerSettingsInPlace: refusing to write over an unreadable client list";
        return errorCode;
    }
    if (updatedClients.isEmpty()) {
        logger.error() << "Xray applyServerSettingsInPlace: no clients to write";
        return ErrorCode::XrayServerNoVlessClients;
    }

    const QString streamClientId = updatedClients[0].toObject().value(px::id).toString();

    const QString listenPort = srv.port.isEmpty() ? QString::fromLatin1(px::defaultPort) : srv.port;

    const QJsonObject serverConfig =
            buildServerConfigJson(srv, updatedClients, streamClientId, realityPrivateKey, realityShortId);

    errorCode = uploadServerConfigAtomically(credentials, container, listenPort, serverConfig);
    if (errorCode != ErrorCode::NoError) {
        logger.error() << "Xray applyServerSettingsInPlace: server config was not applied, error="
                       << static_cast<int>(errorCode) << "port=" << listenPort;
        return errorCode;
    }

    XrayProtocolConfig updated = buildClientProtocolConfig(credentials, container, srv, xrayCfg->clientTemplate, clientId,
                                                           errorCode, realityPublicKey, realityShortId);
    if (errorCode != ErrorCode::NoError) {
        logger.error() << "Xray applyServerSettingsInPlace: buildClientProtocolConfig failed, error="
                       << static_cast<int>(errorCode);
        return errorCode;
    }
    updated.clientTemplate.serverFingerprint = srv.sharedBlockFingerprint();
    updated.clientTemplate.updatedAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    updated.clientTemplate.pendingServerUpload =
            !uploadClientTemplate(credentials, container, updated.clientTemplate);
    containerConfig.protocolConfig = updated;
    return ErrorCode::NoError;
}

QString XrayConfigurator::prepareServerConfig(const ServerCredentials &credentials, DockerContainer container,
                                               const ContainerConfig &containerConfig,
                                               const DnsSettings &dnsSettings,
                                               ErrorCode &errorCode)
{
    ContainerConfig mutableConfig = containerConfig;
    QString clientId;
    const ErrorCode applyError =
            applyServerSettingsToRemote(credentials, container, mutableConfig, dnsSettings, true, &clientId);
    errorCode = applyError;
    if (applyError != ErrorCode::NoError || clientId.isEmpty()) {
        return QString();
    }
    return clientId;
}

bool XrayConfigurator::uploadClientTemplate(const ServerCredentials &credentials, DockerContainer container,
                                            const XrayClientTemplate &clientTemplate) const
{
    namespace px = amnezia::protocols::xray;

    XrayClientTemplate stamped = clientTemplate;
    stamped.updatedAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    const QString json = QString::fromUtf8(QJsonDocument(stamped.toJson()).toJson());
    const ErrorCode errorCode = m_sshSession->uploadTextFileToContainer(
            container, credentials, json, QString::fromLatin1(px::clientTemplatePath),
            libssh::ScpOverwriteMode::ScpOverwriteExisting);
    if (errorCode != ErrorCode::NoError) {
        logger.warning() << "Xray uploadClientTemplate: failed, error=" << static_cast<int>(errorCode)
                         << ", this device is now ahead of the template stored on the server";
        return false;
    }
    return true;
}

XrayClientTemplate XrayConfigurator::readClientTemplate(const ServerCredentials &credentials,
                                                        DockerContainer container, bool &outFound) const
{
    namespace px = amnezia::protocols::xray;

    outFound = false;
    ErrorCode readError = ErrorCode::NoError;
    const QString content = QString::fromUtf8(m_sshSession->getTextFileFromContainer(
            container, credentials, QString::fromLatin1(px::clientTemplatePath), readError));
    if (readError != ErrorCode::NoError || content.trimmed().isEmpty()) {
        return {};
    }

    const QJsonDocument doc = QJsonDocument::fromJson(content.toUtf8());
    if (!doc.isObject()) {
        logger.warning() << "Xray readClientTemplate: template on the server is not valid json";
        return {};
    }

    XrayClientTemplate tpl = XrayClientTemplate::fromJson(doc.object());
    if (tpl.formatVersion == 0) {
        logger.warning() << "Xray readClientTemplate: template on the server has no format version, ignoring it";
        return {};
    }

    outFound = true;
    return tpl;
}

ErrorCode XrayConfigurator::rebuildClientConfigLocally(const ServerCredentials &credentials,
                                                       ContainerConfig &containerConfig) const
{
    namespace px = amnezia::protocols::xray;

    if (credentials.hostName.isEmpty()) {
        logger.error() << "Xray rebuildClientConfigLocally: no host name, refusing to write an unusable config";
        return ErrorCode::InternalError;
    }

    const auto *xrayCfg = containerConfig.protocolConfig.as<XrayProtocolConfig>();
    if (!xrayCfg) {
        logger.error() << "Xray rebuildClientConfigLocally: missing XrayProtocolConfig";
        return ErrorCode::InternalError;
    }
    if (xrayCfg->serverConfig.isThirdPartyConfig) {
        return ErrorCode::NoError;
    }
    if (!xrayCfg->hasClientConfig() || xrayCfg->clientConfig->nativeConfig.isEmpty()) {
        logger.info() << "Xray rebuildClientConfigLocally: nothing stored yet, skipping";
        return ErrorCode::NoError;
    }

    const XrayServerConfig &srv = xrayCfg->serverConfig;

    const QJsonDocument doc = QJsonDocument::fromJson(xrayCfg->clientConfig->nativeConfig.toUtf8());
    if (!doc.isObject()) {
        logger.error() << "Xray rebuildClientConfigLocally: stored config is not valid JSON";
        return ErrorCode::XrayServerConfigInvalid;
    }

    const QJsonArray outbounds = doc.object().value(px::outbounds).toArray();
    if (outbounds.isEmpty()) {
        logger.error() << "Xray rebuildClientConfigLocally: stored config has no outbounds";
        return ErrorCode::XrayServerConfigInvalid;
    }
    const QJsonObject outbound = outbounds[0].toObject();
    const QJsonObject reality = outbound.value(px::streamSettings).toObject().value(px::realitySettings).toObject();
    const QString publicKey = reality.value(px::publicKey).toString();
    const QString shortId = reality.value(px::shortId).toString();

    QString clientId = xrayCfg->clientConfig->id;
    if (clientId.isEmpty()) {
        const QJsonArray vnext = outbound.value(px::settings).toObject().value(px::vnext).toArray();
        if (!vnext.isEmpty()) {
            const QJsonArray users = vnext[0].toObject().value(px::users).toArray();
            if (!users.isEmpty()) {
                clientId = users[0].toObject().value(px::id).toString();
            }
        }
    }
    if (clientId.isEmpty()) {
        logger.error() << "Xray rebuildClientConfigLocally: no client id in the stored config";
        return ErrorCode::XrayServerNoVlessClients;
    }

    if (effectiveSecurity(srv) == QLatin1String("reality") && (publicKey.isEmpty() || shortId.isEmpty())) {
        logger.error() << "Xray rebuildClientConfigLocally: stored config carries no reality keys";
        return ErrorCode::XrayRealityKeysReadFailed;
    }

    ErrorCode errorCode = ErrorCode::NoError;
    XrayProtocolConfig updated =
            buildClientProtocolConfig(credentials, DockerContainer::Xray, srv, xrayCfg->clientTemplate, clientId,
                                      errorCode, publicKey, shortId);
    if (errorCode != ErrorCode::NoError) {
        logger.error() << "Xray rebuildClientConfigLocally: buildClientProtocolConfig failed, error="
                       << static_cast<int>(errorCode);
        return errorCode;
    }

    containerConfig.protocolConfig = updated;
    return ErrorCode::NoError;
}

XrayProtocolConfig XrayConfigurator::buildClientProtocolConfig(const ServerCredentials &credentials,
                                                               DockerContainer container,
                                                               const XrayServerConfig &srv,
                                                               const XrayClientTemplate &tpl, const QString &clientId,
                                                               ErrorCode &errorCode,
                                                               const QString &prefetchedRealityPublicKey,
                                                               const QString &prefetchedRealityShortId) const
{
    QString xrayPublicKey = prefetchedRealityPublicKey;
    QString xrayShortId = prefetchedRealityShortId;

    const QString securityEff = effectiveSecurity(srv);

    if (securityEff == QLatin1String("reality")) {
        if (xrayPublicKey.isEmpty() || xrayShortId.isEmpty()) {
            errorCode = readRealityKeyFiles(container, credentials, xrayPublicKey, xrayShortId);
            if (errorCode != ErrorCode::NoError) {
                return {};
            }
        }
    }

    QJsonObject userObj;
    userObj[amnezia::protocols::xray::id] = clientId;
    userObj[amnezia::protocols::xray::encryption] = QStringLiteral("none");
    const QString flowValue = effectiveClientFlow(srv);
    if (!flowValue.isEmpty()) {
        userObj[amnezia::protocols::xray::flow] = flowValue;
    }

    QJsonObject vnextEntry;
    vnextEntry[amnezia::protocols::xray::address] = credentials.hostName;
    vnextEntry[amnezia::protocols::xray::port] =
            srv.port.isEmpty() ? QString(amnezia::protocols::xray::defaultPort).toInt() : srv.port.toInt();
    vnextEntry[amnezia::protocols::xray::users] = QJsonArray { userObj };

    QJsonObject outboundSettings;
    outboundSettings[amnezia::protocols::xray::vnext] = QJsonArray { vnextEntry };

    QJsonObject outbound;
    outbound[QStringLiteral("protocol")] = QStringLiteral("vless");
    outbound[amnezia::protocols::xray::settings] = outboundSettings;

    QJsonObject streamObj = buildStreamSettings(srv, tpl, clientId);
    if (securityEff == QLatin1String("reality")) {
        QJsonObject rs = streamObj[amnezia::protocols::xray::realitySettings].toObject();
        rs[amnezia::protocols::xray::publicKey] = xrayPublicKey;
        rs[amnezia::protocols::xray::shortId] = xrayShortId;
        rs[amnezia::protocols::xray::spiderX] = QString();
        streamObj[amnezia::protocols::xray::realitySettings] = rs;
    }

    outbound[amnezia::protocols::xray::streamSettings] = streamObj;

    QJsonObject inboundObj;
    inboundObj[QStringLiteral("listen")] = amnezia::protocols::xray::defaultLocalListenAddr;
    inboundObj[amnezia::protocols::xray::port] = amnezia::protocols::xray::defaultLocalProxyPort;
    inboundObj[QStringLiteral("protocol")] = QStringLiteral("socks");
    inboundObj[amnezia::protocols::xray::settings] = QJsonObject { { QStringLiteral("udp"), true } };

    QJsonObject clientJson;
    clientJson[QStringLiteral("log")] = QJsonObject { { QStringLiteral("loglevel"), QStringLiteral("error") } };
    clientJson[amnezia::protocols::xray::inbounds] = QJsonArray { inboundObj };
    clientJson[amnezia::protocols::xray::outbounds] = QJsonArray { outbound };

    const QString config = QString::fromUtf8(QJsonDocument(clientJson).toJson(QJsonDocument::Compact));

    XrayProtocolConfig protocolConfig;
    protocolConfig.serverConfig = srv;
    protocolConfig.clientTemplate = tpl;

    XrayClientConfig clientConfig;
    clientConfig.nativeConfig = config;
    clientConfig.localPort = QString(amnezia::protocols::xray::defaultLocalProxyPort);
    clientConfig.id = clientId;
    clientConfig.templateFingerprint = tpl.contentFingerprint();
    protocolConfig.setClientConfig(clientConfig);

    return protocolConfig;
}

QJsonObject XrayConfigurator::buildStreamSettings(const XrayServerConfig &srv, const XrayClientTemplate &tpl,
                                                  const QString &clientId) const
{
    QJsonObject streamSettings;
    const auto &xhttp = srv.xhttp;
    const auto &mkcp = srv.mkcp;
    namespace px = amnezia::protocols::xray;

    QString networkValue = QStringLiteral("tcp");
    if (srv.transport == QLatin1String("xhttp"))
        networkValue = QStringLiteral("xhttp");
    else if (srv.transport == QLatin1String("mkcp"))
        networkValue = QStringLiteral("kcp");
    streamSettings[px::network] = networkValue;

    const QString securityEff = effectiveSecurity(srv);
    if (securityEff != srv.security) {
        logger.warning() << "Xray buildStreamSettings: security downgraded for the client config, requested="
                         << srv.security << "written=" << securityEff << "transport=" << srv.transport;
    }
    streamSettings[px::security] = securityEff;

    if (securityEff == QLatin1String("tls")) {
        QJsonObject tlsSettings;
        const QString sniEff = srv.sni.isEmpty() ? QString::fromLatin1(px::defaultSni) : srv.sni;
        tlsSettings[px::serverName] = sniEff;
        const QString alpnEff = srv.alpn.isEmpty() ? QString::fromLatin1(px::defaultAlpn) : srv.alpn;
        QJsonArray alpnArray;
        for (const QString &a : alpnEff.split(QLatin1Char(','))) {
            QString t = a.trimmed();
            if (t.isEmpty())
                continue;
            if (t.compare(QLatin1String("HTTP/2"), Qt::CaseInsensitive) == 0)
                t = QStringLiteral("h2");
            else if (t.compare(QLatin1String("HTTP/1.1"), Qt::CaseInsensitive) == 0)
                t = QStringLiteral("http/1.1");
            alpnArray.append(t);
        }
        if (!alpnArray.isEmpty())
            tlsSettings[QStringLiteral("alpn")] = alpnArray;
        const QString fpEff = tpl.fingerprint.isEmpty() ? QString::fromLatin1(px::defaultFingerprint) : tpl.fingerprint;
        tlsSettings[px::fingerprint] = fpEff;
        streamSettings[QStringLiteral("tlsSettings")] = tlsSettings;
    }

    if (securityEff == QLatin1String("reality")) {
        QJsonObject realSettings;
        const QString fpEff = tpl.fingerprint.isEmpty() ? QString::fromLatin1(px::defaultFingerprint) : tpl.fingerprint;
        realSettings[px::fingerprint] = fpEff;
        const QString sniEff = srv.sni.isEmpty() ? QString::fromLatin1(px::defaultSni) : srv.sni;
        realSettings[px::serverName] = sniEff;
        streamSettings[px::realitySettings] = realSettings;
    }

    // XHTTP — JSON must match Xray-core SplitHTTPConfig (flat xPadding fields, see transport_internet.go)
    if (srv.transport == QLatin1String("xhttp")) {
        QJsonObject xo;
        const QString hostEff = xhttp.host.isEmpty() ? QString::fromLatin1(px::defaultXhttpHost) : xhttp.host;
        xo[QStringLiteral("host")] = hostEff;
        if (!xhttp.path.isEmpty())
            xo[QStringLiteral("path")] = xhttp.path;
        QString modeEff = normalizeXhttpMode(xhttp.mode);
        if (modeEff == QLatin1String("auto") || modeEff == QLatin1String("packet-up")) {
            modeEff = QStringLiteral("stream-one");
        }
        xo[QStringLiteral("mode")] = modeEff;

        // No "Host" in headers: xray rejects it when the top-level "host" field is set.

        const QString methodEff =
                tpl.uplinkMethod.isEmpty() ? QString::fromLatin1(px::defaultXhttpUplinkMethod) : tpl.uplinkMethod;
        xo[QStringLiteral("uplinkHTTPMethod")] = methodEff.toUpper();

        xo[QStringLiteral("noGRPCHeader")] = xhttp.disableGrpc;
        xo[QStringLiteral("noSSEHeader")] = xhttp.disableSse;

        const QString sessPl = normalizeSessionSeqPlacement(xhttp.sessionPlacement);
        if (!sessPl.isEmpty())
            xo[QStringLiteral("sessionIDPlacement")] = sessPl;
        const QString seqPl = normalizeSessionSeqPlacement(xhttp.seqPlacement);
        if (!seqPl.isEmpty())
            xo[QStringLiteral("seqPlacement")] = seqPl;
        if (!xhttp.sessionKey.isEmpty())
            xo[QStringLiteral("sessionIDKey")] = xhttp.sessionKey;
        if (!xhttp.seqKey.isEmpty())
            xo[QStringLiteral("seqKey")] = xhttp.seqKey;

        const QString uDataPl = normalizeUplinkDataPlacement(xhttp.uplinkDataPlacement);
        const bool uDataNeedsPacketUp =
                uDataPl == QLatin1String("header") || uDataPl == QLatin1String("cookie");
        if (!(uDataNeedsPacketUp && modeEff != QLatin1String("packet-up")))
            xo[QStringLiteral("uplinkDataPlacement")] = uDataPl;
        if (!xhttp.uplinkDataKey.isEmpty())
            xo[QStringLiteral("uplinkDataKey")] = xhttp.uplinkDataKey;

        const QString ucs = tpl.uplinkChunkSize.isEmpty() ? QString::fromLatin1(px::defaultXhttpUplinkChunkSize)
                                                         : tpl.uplinkChunkSize;
        if (!ucs.isEmpty() && ucs != QLatin1String("0")) {
            xo[QStringLiteral("uplinkChunkSize")] = ucs.toInt();
        }

        if (!xhttp.scMaxBufferedPosts.isEmpty())
            xo[QStringLiteral("scMaxBufferedPosts")] = xhttp.scMaxBufferedPosts.toLongLong();

        putIntRangeIfAny(xo, "scMaxEachPostBytes", xhttp.scMaxEachPostBytesMin, xhttp.scMaxEachPostBytesMax,
                         px::defaultXhttpScMaxEachPostBytesMin, px::defaultXhttpScMaxEachPostBytesMax);
        putIntRangeIfAny(xo, "scMinPostsIntervalMs", tpl.scMinPostsIntervalMsMin, tpl.scMinPostsIntervalMsMax,
                         px::defaultXhttpScMinPostsIntervalMsMin, px::defaultXhttpScMinPostsIntervalMsMax);
        putIntRangeIfAny(xo, "scStreamUpServerSecs", xhttp.scStreamUpServerSecsMin, xhttp.scStreamUpServerSecsMax,
                         px::defaultXhttpScStreamUpServerSecsMin, px::defaultXhttpScStreamUpServerSecsMax);

        const auto &pad = xhttp.xPadding;
        xo[QStringLiteral("xPaddingObfsMode")] = pad.obfsMode;
        if (pad.obfsMode) {
            if (!pad.bytesMin.isEmpty() || !pad.bytesMax.isEmpty()) {
                const int fromV = pad.bytesMin.isEmpty()
                        ? QString::fromLatin1(px::defaultXPaddingBytesMin).toInt()
                        : pad.bytesMin.toInt();
                int toV = pad.bytesMax.isEmpty()
                        ? QString::fromLatin1(px::defaultXPaddingBytesMax).toInt()
                        : pad.bytesMax.toInt();
                if (toV < fromV)
                    toV = fromV;
                xo[QStringLiteral("xPaddingBytes")] = makeRangeString(QString::number(fromV), QString::number(toV));
            }
            xo[QStringLiteral("xPaddingKey")] =
                    pad.key.isEmpty() ? QString::fromLatin1(px::defaultXPaddingKey) : pad.key;
            xo[QStringLiteral("xPaddingHeader")] =
                    pad.header.isEmpty() ? QString::fromLatin1(px::defaultXPaddingHeader) : pad.header;
            xo[QStringLiteral("xPaddingPlacement")] = normalizeXPaddingPlacement(
                    pad.placement.isEmpty() ? QString::fromLatin1(px::defaultXPaddingPlacement) : pad.placement);
            xo[QStringLiteral("xPaddingMethod")] = normalizeXPaddingMethod(
                    pad.method.isEmpty() ? QString::fromLatin1(px::defaultXPaddingMethod) : pad.method);
        }

        // xmux: Xray has no "enabled" flag; omit object when UI disables multiplex tuning.
        if (tpl.xmux.enabled) {
            QJsonObject mux;
            auto addMuxRange = [&](const char *key, const QString &a, const QString &b) {
                // omit empty / 0-0 ranges (xray may reject "0-0")
                const bool aZero = a.isEmpty() || a == QLatin1String("0");
                const bool bZero = b.isEmpty() || b == QLatin1String("0");
                if (aZero && bZero)
                    return;
                const QString aV = a.isEmpty() ? QStringLiteral("0") : a;
                const QString bV = b.isEmpty() ? QStringLiteral("0") : b;
                mux[QString::fromUtf8(key)] = makeRangeString(aV, bV);
            };
            addMuxRange("maxConcurrency", tpl.xmux.maxConcurrencyMin, tpl.xmux.maxConcurrencyMax);
            addMuxRange("maxConnections", tpl.xmux.maxConnectionsMin, tpl.xmux.maxConnectionsMax);
            addMuxRange("cMaxReuseTimes", tpl.xmux.cMaxReuseTimesMin, tpl.xmux.cMaxReuseTimesMax);
            addMuxRange("hMaxRequestTimes", tpl.xmux.hMaxRequestTimesMin, tpl.xmux.hMaxRequestTimesMax);
            addMuxRange("hMaxReusableSecs", tpl.xmux.hMaxReusableSecsMin, tpl.xmux.hMaxReusableSecsMax);
            if (!tpl.xmux.hKeepAlivePeriod.isEmpty())
                mux[QStringLiteral("hKeepAlivePeriod")] = tpl.xmux.hKeepAlivePeriod.toLongLong();
            if (!mux.isEmpty())
                xo[QStringLiteral("xmux")] = mux;
        }

        streamSettings[QStringLiteral("xhttpSettings")] = xo;
    }

    if (srv.transport == QLatin1String("mkcp")) {
        QJsonObject kcpObj;
        const QString ttiEff = mkcp.tti.isEmpty() ? QString::fromLatin1(px::defaultMkcpTti) : mkcp.tti;
        const QString mtuEff = mkcp.mtu.isEmpty() ? QString::fromLatin1(px::defaultMkcpMtu) : mkcp.mtu;
        const QString upEff = mkcp.uplinkCapacity.isEmpty() ? QString::fromLatin1(px::defaultMkcpUplinkCapacity)
                                                            : mkcp.uplinkCapacity;
        const QString downEff = mkcp.downlinkCapacity.isEmpty() ? QString::fromLatin1(px::defaultMkcpDownlinkCapacity)
                                                                : mkcp.downlinkCapacity;
        const QString cwndEff = mkcp.cwndMultiplier.isEmpty() ? QString::fromLatin1(px::defaultMkcpCwndMultiplier)
                                                              : mkcp.cwndMultiplier;
        kcpObj[QStringLiteral("tti")] = ttiEff.toInt();
        kcpObj[QStringLiteral("mtu")] = mtuEff.toInt();
        kcpObj[QStringLiteral("uplinkCapacity")] = upEff.toInt();
        kcpObj[QStringLiteral("downlinkCapacity")] = downEff.toInt();
        kcpObj[QStringLiteral("cwndMultiplier")] = cwndEff.toInt();
        if (!mkcp.maxSendingWindow.isEmpty() && mkcp.maxSendingWindow.toInt() >= mtuEff.toInt()) {
            kcpObj[QStringLiteral("maxSendingWindow")] = mkcp.maxSendingWindow.toInt();
        }
        streamSettings[QStringLiteral("kcpSettings")] = kcpObj;
    }

    return streamSettings;
}

ProtocolConfig XrayConfigurator::createConfig(const ServerCredentials &credentials, DockerContainer container,
                                              const ContainerConfig &containerConfig,
                                              const DnsSettings &dnsSettings,
                                              ErrorCode &errorCode)
{
    if (const auto *xrayCfg = containerConfig.protocolConfig.as<XrayProtocolConfig>()) {
        if (xrayCfg->serverConfig.isThirdPartyConfig && xrayCfg->hasClientConfig()) {
            logger.info() << "Xray createConfig: returning existing third-party client config without server SSH";
            return *xrayCfg;
        }
    }

    const XrayServerConfig *serverConfig = nullptr;
    XrayClientTemplate clientTemplate;
    if (const auto *xrayCfg = containerConfig.protocolConfig.as<XrayProtocolConfig>()) {
        serverConfig = &xrayCfg->serverConfig;
        clientTemplate = xrayCfg->clientTemplate;
    }

    if (!serverConfig) {
        logger.error() << "No XrayProtocolConfig found";
        errorCode = ErrorCode::InternalError;
        return XrayProtocolConfig{};
    }

    const XrayServerConfig &srv = *serverConfig;

    QString xrayClientId = prepareServerConfig(credentials, container, containerConfig, dnsSettings, errorCode);
    if (errorCode != ErrorCode::NoError || xrayClientId.isEmpty()) {
        logger.error() << "Failed to prepare server config";
        if (errorCode == ErrorCode::NoError) {
            errorCode = ErrorCode::InternalError;
        }
        return XrayProtocolConfig{};
    }

    return buildClientProtocolConfig(credentials, container, srv, clientTemplate, xrayClientId, errorCode);
}