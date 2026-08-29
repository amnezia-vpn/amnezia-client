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

    QString tlsFingerprintFromEnsureOutput(const QString &stdOut)
    {
        const QString prefix = QStringLiteral("amnezia_tls_fp=");
        const auto lines = stdOut.split(QLatin1Char('\n'));
        for (QString line : lines) {
            line = line.trimmed();
            if (!line.startsWith(prefix))
                continue;
            QString fp = line.mid(prefix.size());
            fp.remove(QLatin1Char(':'));
            fp.remove(QLatin1Char(' '));
            fp = fp.toLower();
            if (fp.size() != 64)
                return {};
            for (const QChar ch : fp) {
                const ushort u = ch.unicode();
                const bool hex = (u >= '0' && u <= '9') || (u >= 'a' && u <= 'f');
                if (!hex)
                    return {};
            }
            return fp;
        }
        return {};
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
        logger.info() << "Xray applyServerSettings: skipped (third-party/native profile)";
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
    logger.info() << "Xray applyServerSettings: read server config, bytes=" << currentConfig.size();

    QJsonDocument doc = QJsonDocument::fromJson(currentConfig.toUtf8());
    if (doc.isNull() || !doc.isObject()) {
        logger.error() << "Failed to parse server config JSON";
        return ErrorCode::XrayServerConfigInvalid;
    }

    QJsonObject serverConfig = doc.object();
    const QJsonObject originalConfig = serverConfig;

    QJsonArray clients = XrayServerConfig::clientsFromServerInboundJson(serverConfig);
    QString clientId;

    if (appendNewClient) {
        const bool adoptingOwnAccount = !(xrayCfg->hasClientConfig() && !xrayCfg->clientConfig->id.isEmpty());

        QString ownClientId;
        if (readContainerKeyFile(container, credentials, QString::fromLatin1(amnezia::protocols::xray::uuidPath),
                                 ownClientId)
            != ErrorCode::NoError) {
            ownClientId.clear();
        }

        const int existingIndex =
                adoptingOwnAccount ? XrayServerConfig::indexOfClient(clients, ownClientId) : -1;

        if (existingIndex >= 0) {
            clientId = ownClientId;
            clients[existingIndex] =
                    XrayServerConfig::applyFlowToClient(clients[existingIndex].toObject(), flowValue);
        } else {
            const bool takeOverOwnAccount = adoptingOwnAccount && !ownClientId.isEmpty();
            clientId = takeOverOwnAccount ? ownClientId : QUuid::createUuid().toString(QUuid::WithoutBraces);
            clients.append(XrayServerConfig::makeClientEntry(clientId, flowValue));
        }
    } else {
        if (clients.isEmpty()) {
            logger.error() << "Server config has no VLESS clients";
            return ErrorCode::XrayServerNoVlessClients;
        }
        clientId = XrayServerConfig::firstClientId(clients);
        if (clientId.isEmpty()) {
            logger.error() << "Server config VLESS client has empty id";
            return ErrorCode::XrayServerNoVlessClients;
        }
        clients = XrayServerConfig::applyFlowToClients(clients, flowValue);
    }

    const XrayServerJsonStatus writeStatus =
            XrayServerConfig::setClientsInServerInboundJson(serverConfig, clients);
    if (writeStatus != XrayServerJsonStatus::Ok) {
        return ErrorCode::XrayServerConfigInvalid;
    }

    const QString listenPort = srv.port.isEmpty() ? QString::fromLatin1(amnezia::protocols::xray::defaultPort)
                                                  : srv.port;
    if (serverConfig != originalConfig) {
        errorCode = uploadServerConfigAtomically(credentials, container, listenPort, serverConfig);
        if (errorCode != ErrorCode::NoError) {
            return errorCode;
        }
    }
    logger.info() << "Xray applyServerSettings: server config uploaded and container restarted";

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
    logger.info() << "Xray applyServerSettings: done, clientId=" << clientId;
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
                                                      ContainerConfig &containerConfig, const DnsSettings &dnsSettings,
                                                      bool useAtomicApply)
{
    Q_UNUSED(dnsSettings);

    const auto *xrayCfg = containerConfig.protocolConfig.as<XrayProtocolConfig>();
    if (!xrayCfg) {
        logger.error() << "Xray writeServerConfigForSetup: missing XrayProtocolConfig";
        return ErrorCode::InternalError;
    }
    const XrayServerConfig &srv = xrayCfg->serverConfig;
    if (srv.isThirdPartyConfig) {
        logger.info() << "Xray writeServerConfigForSetup: skipped (third-party/native profile)";
        return ErrorCode::NoError;
    }

    ErrorCode errorCode = ErrorCode::NoError;

    QString clientId;
    errorCode = readContainerKeyFile(container, credentials, QString::fromLatin1(amnezia::protocols::xray::uuidPath), clientId);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    const QString securityEff = effectiveSecurity(srv);
    QString tlsPin;
    if (securityEff == QLatin1String("tls")) {
        errorCode = ensureTlsCertificate(credentials, container, tlsPin);
        if (errorCode != ErrorCode::NoError) {
            return errorCode;
        }
    }

    QString realityPrivateKey;
    QString realityPublicKey;
    QString realityShortId;
    if (securityEff == QLatin1String("reality")) {
        errorCode = readContainerKeyFile(container, credentials, QString::fromLatin1(amnezia::protocols::xray::PrivateKeyPath), realityPrivateKey);
        if (errorCode != ErrorCode::NoError)
            return errorCode;
        errorCode = readContainerKeyFile(container, credentials, QString::fromLatin1(amnezia::protocols::xray::PublicKeyPath), realityPublicKey);
        if (errorCode != ErrorCode::NoError)
            return errorCode;
        errorCode = readContainerKeyFile(container, credentials, QString::fromLatin1(amnezia::protocols::xray::shortidPath), realityShortId);
        if (errorCode != ErrorCode::NoError)
            return errorCode;
    }

    const QString flowValue = effectiveClientFlow(srv);
    const QJsonArray clients = collectServerClients(credentials, container, flowValue, clientId, errorCode);
    if (errorCode != ErrorCode::NoError) {
        logger.error() << "Xray writeServerConfigForSetup: upload failed, error=" << static_cast<int>(errorCode);
        return errorCode;
    }

    const QString portEff = srv.port.isEmpty() ? QString::fromLatin1(amnezia::protocols::xray::defaultPort) : srv.port;


    XrayServerInboundInputs inputs;
    inputs.clients = clients;
    inputs.realityPrivateKey = realityPrivateKey;
    inputs.realityShortId = realityShortId;

    const QJsonObject inboundJson = srv.toServerInboundJson(inputs);
    if (useAtomicApply) {
        errorCode = uploadServerConfigAtomically(credentials, container, portEff, inboundJson);
        if (errorCode != ErrorCode::NoError) {
            return errorCode;
        }
    } else {
        const QString json = QString::fromUtf8(QJsonDocument(inboundJson).toJson());
        errorCode = m_sshSession->uploadTextFileToContainer(container, credentials, json,
                                                            QString::fromLatin1(amnezia::protocols::xray::serverConfigPath),
                                                            libssh::ScpOverwriteMode::ScpOverwriteExisting);
        if (errorCode != ErrorCode::NoError) {
            return errorCode;
        }
    }

    XrayProtocolConfig updated =
            buildClientProtocolConfig(credentials, container, srv, xrayCfg->clientTemplate, clientId, errorCode,
                                      realityPublicKey, realityShortId, tlsPin);
    if (errorCode != ErrorCode::NoError) {
        logger.error() << "Xray writeServerConfigForSetup: buildClientProtocolConfig failed, error="
                       << static_cast<int>(errorCode);
        return errorCode;
    }
    updated.clientTemplate.updatedAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    if (!uploadClientTemplate(credentials, container, updated.clientTemplate)) {
        logger.warning() << "Xray writeServerConfigForSetup: uploadClientTemplate failed; server template may be stale";
    }
    containerConfig.protocolConfig = updated;
    logger.info() << "Xray writeServerConfigForSetup: done, clientId=" << clientId;
    return ErrorCode::NoError;
}

QJsonArray XrayConfigurator::collectServerClients(const ServerCredentials &credentials, DockerContainer container,
                                                   const QString &flowValue, const QString &fallbackClientId,
                                                   ErrorCode &outError) const
{
    outError = ErrorCode::NoError;
    const QString configPath = QString::fromLatin1(amnezia::protocols::xray::serverConfigPath);

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
            outError = ErrorCode::XrayServerConfigInvalid;
            return {};
        }
        if (probeOut.contains(QLatin1String("amnezia_config=present"))) {
            outError = ErrorCode::XrayServerConfigInvalid;
            return {};
        }
    } else {
        const QJsonDocument doc = QJsonDocument::fromJson(currentConfig.toUtf8());
        if (!doc.isObject()) {
            outError = ErrorCode::XrayServerConfigInvalid;
            return {};
        }
        existing = XrayServerConfig::clientsFromServerInboundJson(doc.object());
    }

    QJsonArray clients =
            XrayServerConfig::applyFlowToClients(existing, flowValue, XrayClientListFilter::DropWithoutId);

    if (!fallbackClientId.isEmpty()
        && XrayServerConfig::indexOfClient(clients, fallbackClientId) < 0) {
        clients.append(XrayServerConfig::makeClientEntry(fallbackClientId, flowValue));
    }

    return clients;
}

ErrorCode XrayConfigurator::ensureTlsCertificate(const ServerCredentials &credentials, DockerContainer container,
                                                 QString &outFingerprint) const
{
    outFingerprint.clear();

    QString stdOut;
    auto collect = [&stdOut](const QString &data, libssh::Client &) {
        stdOut += data + QLatin1Char('\n');
        return ErrorCode::NoError;
    };

    const QString certPath = QString::fromLatin1(amnezia::protocols::xray::tlsCertPath);
    const QString keyPath = QString::fromLatin1(amnezia::protocols::xray::tlsKeyPath);
    const QString script = QStringLiteral(
            "has_key() { [ -f \"$1\" ] && [ -n \"$(tr -d '[:space:]' < \"$1\" 2>/dev/null)\" ]; }\n"
            "CERT=%1\n"
            "KEY=%2\n"
            "if has_key \"$CERT\" && has_key \"$KEY\"; then echo amnezia_tls=present;\n"
            "else\n"
            "  openssl req -x509 -newkey rsa:2048 -nodes -days 3650 -subj \"/CN=amnezia-xray\" "
            "-keyout \"$KEY\" -out \"$CERT\" || true\n"
            "  if has_key \"$CERT\" && has_key \"$KEY\"; then echo amnezia_tls=created;\n"
            "  else echo amnezia_tls=failed; exit 1; fi\n"
            "fi\n"
            "FP=$(openssl x509 -noout -fingerprint -sha256 -in \"$CERT\" 2>/dev/null "
            "| sed 's/.*Fingerprint=//' | tr -d ':' | tr 'A-F' 'a-f')\n"
            "if [ -z \"$FP\" ]; then echo amnezia_tls=failed; exit 1; fi\n"
            "echo amnezia_tls_fp=$FP\n"
            "exit 0\n")
            .arg(certPath, keyPath);

    const ErrorCode scriptError = m_sshSession->runContainerScript(credentials, container, script, collect, collect);
    const bool present = stdOut.contains(QLatin1String("amnezia_tls=present"));
    const bool created = stdOut.contains(QLatin1String("amnezia_tls=created"));
    outFingerprint = tlsFingerprintFromEnsureOutput(stdOut);
    if (scriptError != ErrorCode::NoError || (!present && !created) || outFingerprint.isEmpty()) {
        outFingerprint.clear();
        return ErrorCode::XrayTlsNotSupported;
    }
    return ErrorCode::NoError;
}

ErrorCode XrayConfigurator::uploadServerConfigAtomically(const ServerCredentials &credentials,
                                                         DockerContainer container, const QString &listenPort,
                                                         const QJsonObject &serverConfig) const
{
    const QString livePath = QString::fromLatin1(amnezia::protocols::xray::serverConfigPath);
    const QString stagedPath = QStringLiteral("/opt/amnezia/xray/server.new.json");

    const QString json = QString::fromUtf8(QJsonDocument(serverConfig).toJson());
    ErrorCode errorCode = m_sshSession->uploadTextFileToContainer(container, credentials, json, stagedPath,
                                                                  libssh::ScpOverwriteMode::ScpOverwriteExisting);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    const QString swapScript = QStringLiteral(
            "LIVE=%1\n"
            "STAGED=%2\n"
            "BACKUP=\"$LIVE.bak\"\n"
            "if [ ! -s \"$STAGED\" ]; then echo \"amnezia_apply=missing\"; exit 0; fi\n"
            "if ! xray -test -format json -config \"$STAGED\" > /tmp/xray_test.log 2>&1; then\n"
            "  echo \"amnezia_apply=invalid\"\n"
            "  rm -f \"$STAGED\"\n"
            "  exit 0\n"
            "fi\n"
            "if [ -f \"$LIVE\" ]; then cp \"$LIVE\" \"$BACKUP\"; fi\n"
            "mv \"$STAGED\" \"$LIVE\"\n"
            "echo \"amnezia_apply=swapped\"\n")
                                       .arg(livePath, stagedPath);

    QString swapOut;
    auto collectSwap = [&swapOut](const QString &data, libssh::Client &) {
        swapOut += data + "\n";
        return ErrorCode::NoError;
    };
    errorCode = m_sshSession->runContainerScript(credentials, container, swapScript, collectSwap, collectSwap);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }
    if (swapOut.contains(QLatin1String("amnezia_apply=missing"))) {
        return ErrorCode::XrayServerConfigRejected;
    }
    if (swapOut.contains(QLatin1String("amnezia_apply=invalid"))) {
        return ErrorCode::XrayServerConfigRejected;
    }
    if (!swapOut.contains(QLatin1String("amnezia_apply=swapped"))) {
        return ErrorCode::XrayServerConfigInvalid;
    }

    if (restartXrayContainer(credentials, container) && xrayProcessIsUp(credentials, container)) {
        m_sshSession->runContainerScript(credentials, container,
                                         QStringLiteral("rm -f %1.bak").arg(livePath));
        return ErrorCode::NoError;
    }

    const QString restoreScript = QStringLiteral(
            "LIVE=%1\n"
            "BACKUP=\"$LIVE.bak\"\n"
            "if [ -f \"$BACKUP\" ]; then cp \"$BACKUP\" \"$LIVE\"; echo amnezia_restore=done; "
            "else echo amnezia_restore=nobackup; fi\n")
                                       .arg(livePath);
    QString restoreOut;
    auto collectRestore = [&restoreOut](const QString &data, libssh::Client &) {
        restoreOut += data + "\n";
        return ErrorCode::NoError;
    };
    m_sshSession->runContainerScript(credentials, container, restoreScript, collectRestore, collectRestore);
    if (!restoreOut.contains(QLatin1String("amnezia_restore=done"))) {
        return ErrorCode::XrayServerNotServing;
    }
    if (restartXrayContainer(credentials, container) && xrayProcessIsUp(credentials, container)) {
        return ErrorCode::XrayServerConfigRolledBack;
    }
    return ErrorCode::XrayServerNotServing;
}

bool XrayConfigurator::restartXrayContainer(const ServerCredentials &credentials, DockerContainer container) const
{
    const ErrorCode errorCode = m_sshSession->runScript(
            credentials,
            SshSession::replaceVars(QStringLiteral("sudo docker restart $CONTAINER_NAME"),
                                    amnezia::genBaseVars(credentials, container, QString(), QString())));
    if (errorCode != ErrorCode::NoError) {
        return false;
    }
    return true;
}

bool XrayConfigurator::xrayProcessIsUp(const ServerCredentials &credentials, DockerContainer container) const
{
    const QString script = QStringLiteral(
            "for i in 1 2 3 4 5 6; do\n"
            "  if pgrep xray >/dev/null 2>&1 || ps 2>/dev/null | grep -v grep | grep -q \"[x]ray\"; then\n"
            "    echo amnezia_serving=yes\n"
            "    exit 0\n"
            "  fi\n"
            "  sleep 1\n"
            "done\n"
            "echo amnezia_serving=no\n");
    QString out;
    auto collect = [&out](const QString &data, libssh::Client &) {
        out += data + "\n";
        return ErrorCode::NoError;
    };
    const ErrorCode errorCode = m_sshSession->runContainerScript(credentials, container, script, collect, collect);
    if (errorCode != ErrorCode::NoError) {
        return false;
    }
    return out.contains(QLatin1String("amnezia_serving=yes"));
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
    XrayClientTemplate stamped = clientTemplate;
    stamped.updatedAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    const QJsonObject templateJson = stamped.toJson();
    const QString json = QString::fromUtf8(QJsonDocument(templateJson).toJson());
    const ErrorCode errorCode = m_sshSession->uploadTextFileToContainer(
            container, credentials, json, QString::fromLatin1(amnezia::protocols::xray::clientTemplatePath),
            libssh::ScpOverwriteMode::ScpOverwriteExisting);
    if (errorCode != ErrorCode::NoError) {
        return false;
    }
    return true;
}

XrayClientTemplate XrayConfigurator::readClientTemplate(const ServerCredentials &credentials,
                                                        DockerContainer container, bool &outFound) const
{
    outFound = false;
    ErrorCode readError = ErrorCode::NoError;
    const QString content = QString::fromUtf8(m_sshSession->getTextFileFromContainer(
            container, credentials, QString::fromLatin1(amnezia::protocols::xray::clientTemplatePath), readError));
    if (readError != ErrorCode::NoError || content.trimmed().isEmpty()) {
        return {};
    }

    const QJsonDocument doc = QJsonDocument::fromJson(content.toUtf8());
    if (!doc.isObject()) {
        return {};
    }

    XrayClientTemplate tpl = XrayClientTemplate::fromJson(doc.object());
    if (tpl.formatVersion == 0) {
        return {};
    }

    outFound = true;
    return tpl;
}
XrayProtocolConfig XrayConfigurator::buildClientProtocolConfig(const ServerCredentials &credentials,
                                                               DockerContainer container,
                                                               const XrayServerConfig &srv,
                                                               const XrayClientTemplate &tpl, const QString &clientId,
                                                               ErrorCode &errorCode,
                                                               const QString &prefetchedRealityPublicKey,
                                                               const QString &prefetchedRealityShortId,
                                                               const QString &prefetchedTlsPin) const
{
    QString xrayPublicKey = prefetchedRealityPublicKey;
    QString xrayShortId = prefetchedRealityShortId;
    QString tlsPin = prefetchedTlsPin;

    const QString securityEff = effectiveSecurity(srv);


    if (securityEff == QLatin1String(amnezia::protocols::xray::securityReality)) {
        if (xrayPublicKey.isEmpty() || xrayShortId.isEmpty()) {
            errorCode = readRealityKeyFiles(container, credentials, xrayPublicKey, xrayShortId);
            if (errorCode != ErrorCode::NoError) {
                return {};
            }
        }
    }

    if (securityEff == QLatin1String(amnezia::protocols::xray::securityTls) && !srv.isThirdPartyConfig) {
        if (tlsPin.isEmpty()) {
            errorCode = ensureTlsCertificate(credentials, container, tlsPin);
            if (errorCode != ErrorCode::NoError) {
                return {};
            }
        }
        if (tlsPin.isEmpty()) {
            errorCode = ErrorCode::XrayTlsNotSupported;
            return {};
        }
    }


    XrayProtocolConfig protocolConfig;
    protocolConfig.serverConfig = srv;
    protocolConfig.clientTemplate = tpl;

    XrayClientOutboundInputs inputs;
    inputs.serverAddress = credentials.hostName;
    inputs.clientId = clientId;
    inputs.realityPublicKey = xrayPublicKey;
    inputs.realityShortId = xrayShortId;
    inputs.tlsPinnedPeerCertSha256 = tlsPin;

    XrayClientConfig clientConfig;
    clientConfig.nativeConfig = QString::fromUtf8(
            QJsonDocument(protocolConfig.toClientOutboundJson(inputs)).toJson(QJsonDocument::Compact));
    clientConfig.localPort = QString::fromLatin1(amnezia::protocols::xray::defaultLocalProxyPort);
    clientConfig.id = clientId;
    clientConfig.templateFingerprint = tpl.contentFingerprint();
    protocolConfig.setClientConfig(clientConfig);

    return protocolConfig;
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