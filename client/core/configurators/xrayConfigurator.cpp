#include "xrayConfigurator.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
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

    QString normalizeXhttpMode(const QString &m) {
        const QString t = m.trimmed();
        if (t.isEmpty() || t.compare(QLatin1String("Auto"), Qt::CaseInsensitive) == 0) {
            return QStringLiteral("auto");
        }
        if (t.compare(QLatin1String("Packet-up"), Qt::CaseInsensitive) == 0)
            return QStringLiteral("packet-up");
        if (t.compare(QLatin1String("Stream-up"), Qt::CaseInsensitive) == 0)
            return QStringLiteral("stream-up");
        if (t.compare(QLatin1String("Stream-one"), Qt::CaseInsensitive) == 0)
            return QStringLiteral("stream-one");
        return t.toLower();
    }

    // Xray-core: empty → path; "None" in UI → omit (core default path)
    QString normalizeSessionSeqPlacement(const QString &p)
    {
        if (p.isEmpty() || p.compare(QLatin1String("None"), Qt::CaseInsensitive) == 0)
            return {};
        return p.toLower();
    }

    QString normalizeUplinkDataPlacement(const QString &p)
    {
        if (p.isEmpty() || p.compare(QLatin1String("Body"), Qt::CaseInsensitive) == 0)
            return QStringLiteral("body");
        if (p.compare(QLatin1String("Auto"), Qt::CaseInsensitive) == 0)
            return QStringLiteral("auto");
        if (p.compare(QLatin1String("Query"), Qt::CaseInsensitive) == 0)
            // "Query" is not valid for uplink payload in splithttp; closest documented mode
            return QStringLiteral("header");
        return p.toLower();
    }

    // splithttp: cookie | header | query | queryInHeader (not "body")
    QString normalizeXPaddingPlacement(const QString &p)
    {
        QString t = p.trimmed();
        if (t.isEmpty())
            return QString::fromLatin1(amnezia::protocols::xray::defaultXPaddingPlacement).toLower();
        if (t.compare(QLatin1String("Body"), Qt::CaseInsensitive) == 0)
            return QStringLiteral("queryInHeader");
        if (t.contains(QLatin1String("queryInHeader"), Qt::CaseInsensitive)
            || t.compare(QLatin1String("Query in header"), Qt::CaseInsensitive) == 0)
            return QStringLiteral("queryInHeader");
        return t.toLower();
    }

    // splithttp: repeat-x | tokenish
    QString normalizeXPaddingMethod(const QString &m)
    {
        QString t = m.trimmed();
        if (t.isEmpty() || t.compare(QLatin1String("Repeat-x"), Qt::CaseInsensitive) == 0)
            return QStringLiteral("repeat-x");
        if (t.compare(QLatin1String("Tokenish"), Qt::CaseInsensitive) == 0)
            return QStringLiteral("tokenish");
        if (t.compare(QLatin1String("Random"), Qt::CaseInsensitive) == 0
            || t.compare(QLatin1String("Zero"), Qt::CaseInsensitive) == 0)
            return QStringLiteral("repeat-x");
        return t.toLower();
    }

    void putIntRangeIfAny(QJsonObject &obj, const char *key, QString minV, QString maxV, const char *fallbackMin,
                          const char *fallbackMax)
    {
        if (minV.isEmpty() && maxV.isEmpty())
            return;
        if (minV.isEmpty())
            minV = QString::fromLatin1(fallbackMin);
        if (maxV.isEmpty())
            maxV = QString::fromLatin1(fallbackMax);
        QJsonObject r;
        r[QStringLiteral("from")] = minV.toInt();
        r[QStringLiteral("to")] = maxV.toInt();
        obj[QString::fromUtf8(key)] = r;
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

QJsonObject XrayConfigurator::mergeStreamSettingsForServerInbound(const XrayServerConfig &srv,
                                                                  const QJsonObject &existingStreamSettings) const
{
    QJsonObject streamSettings = buildStreamSettings(srv, QString());

    if (srv.security != QLatin1String("reality")) {
        return streamSettings;
    }

    const QJsonObject newRs = streamSettings[amnezia::protocols::xray::realitySettings].toObject();
    QJsonObject oldRs = existingStreamSettings[amnezia::protocols::xray::realitySettings].toObject();
    QJsonObject merged = oldRs.isEmpty() ? newRs : oldRs;

    const QString siteEff = srv.site.isEmpty() ? QString::fromLatin1(amnezia::protocols::xray::defaultSite) : srv.site;
    const QString sniEff = srv.sni.isEmpty() ? siteEff : srv.sni;

    if (newRs.contains(amnezia::protocols::xray::fingerprint)) {
        merged[amnezia::protocols::xray::fingerprint] = newRs[amnezia::protocols::xray::fingerprint];
    }
    merged[amnezia::protocols::xray::serverNames] = QJsonArray { sniEff };
    if (!merged.contains(QStringLiteral("dest"))) {
        merged[QStringLiteral("dest")] = siteEff + QStringLiteral(":443");
    }

    streamSettings[amnezia::protocols::xray::realitySettings] = merged;
    return streamSettings;
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

    logger.info() << "Xray applyServerSettings: start"
                    << "container=" << static_cast<int>(container) << "host=" << credentials.hostName
                    << "transport=" << srv.transport << "security=" << srv.security << "port=" << srv.port
                    << "appendClient=" << appendNewClient;
    const QString flowValue = srv.flow;
    QString realityPublicKey;
    QString realityShortId;
    if (srv.security == QLatin1String("reality")) {
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

    const QJsonObject existingStream = inbound[amnezia::protocols::xray::streamSettings].toObject();
    inbound[amnezia::protocols::xray::streamSettings] = mergeStreamSettingsForServerInbound(srv, existingStream);

    if (!srv.port.isEmpty()) {
        inbound[amnezia::protocols::xray::port] = srv.port.toInt();
    }

    QJsonObject settings = inbound[amnezia::protocols::xray::settings].toObject();
    if (!settings.contains(amnezia::protocols::xray::clients)) {
        settings[amnezia::protocols::xray::clients] = QJsonArray {};
    }

    QJsonArray clients = settings[amnezia::protocols::xray::clients].toArray();
    QString clientId;

    if (appendNewClient) {
        clientId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        QJsonObject clientEntry;
        clientEntry[amnezia::protocols::xray::id] = clientId;
        if (!flowValue.isEmpty()) {
            clientEntry[amnezia::protocols::xray::flow] = flowValue;
        }
        clients.append(clientEntry);
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

    errorCode = uploadServerConfigJson(credentials, container, dnsSettings, serverConfig);
    if (errorCode != ErrorCode::NoError) {
        logger.error() << "Xray applyServerSettings: upload/restart failed, error=" << static_cast<int>(errorCode);
        return errorCode;
    }
    logger.info() << "Xray applyServerSettings: server config uploaded and container restarted";

    if (outClientId) {
        *outClientId = clientId;
    }

    XrayProtocolConfig updated =
            buildClientProtocolConfig(credentials, container, srv, clientId, errorCode, realityPublicKey, realityShortId);
    if (errorCode != ErrorCode::NoError) {
        logger.error() << "Xray applyServerSettings: buildClientProtocolConfig failed, error="
                       << static_cast<int>(errorCode);
        return errorCode;
    }
    containerConfig.protocolConfig = updated;
    logger.info() << "Xray applyServerSettings: done, clientId=" << clientId;
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

XrayProtocolConfig XrayConfigurator::buildClientProtocolConfig(const ServerCredentials &credentials,
                                                               DockerContainer container,
                                                               const XrayServerConfig &srv, const QString &clientId,
                                                               ErrorCode &errorCode,
                                                               const QString &prefetchedRealityPublicKey,
                                                               const QString &prefetchedRealityShortId) const
{
    QString xrayPublicKey = prefetchedRealityPublicKey;
    QString xrayShortId = prefetchedRealityShortId;

    if (srv.security == QLatin1String("reality")) {
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
    if (!srv.flow.isEmpty()) {
        userObj[amnezia::protocols::xray::flow] = srv.flow;
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

    QJsonObject streamObj = buildStreamSettings(srv, clientId);
    if (srv.security == QLatin1String("reality")) {
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

    XrayClientConfig clientConfig;
    clientConfig.nativeConfig = config;
    clientConfig.localPort = QString(amnezia::protocols::xray::defaultLocalProxyPort);
    clientConfig.id = clientId;
    protocolConfig.setClientConfig(clientConfig);

    return protocolConfig;
}

QJsonObject XrayConfigurator::buildStreamSettings(const XrayServerConfig &srv, const QString &clientId) const
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

    streamSettings[px::security] = srv.security;

    if (srv.security == QLatin1String("tls")) {
        QJsonObject tlsSettings;
        const QString sniEff = srv.sni.isEmpty() ? QString::fromLatin1(px::defaultSni) : srv.sni;
        tlsSettings[px::serverName] = sniEff;
        const QString alpnEff = srv.alpn.isEmpty() ? QString::fromLatin1(px::defaultAlpn) : srv.alpn;
        QJsonArray alpnArray;
        for (const QString &a : alpnEff.split(QLatin1Char(','))) {
            const QString t = a.trimmed();
            if (!t.isEmpty())
                alpnArray.append(t);
        }
        if (!alpnArray.isEmpty())
            tlsSettings[QStringLiteral("alpn")] = alpnArray;
        const QString fpEff = srv.fingerprint.isEmpty() ? QString::fromLatin1(px::defaultFingerprint) : srv.fingerprint;
        tlsSettings[px::fingerprint] = fpEff;
        streamSettings[QStringLiteral("tlsSettings")] = tlsSettings;
    }

    if (srv.security == QLatin1String("reality")) {
        QJsonObject realSettings;
        const QString fpEff = srv.fingerprint.isEmpty() ? QString::fromLatin1(px::defaultFingerprint) : srv.fingerprint;
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
        xo[QStringLiteral("mode")] = normalizeXhttpMode(xhttp.mode);

        if (xhttp.headersTemplate.compare(QLatin1String("HTTP"), Qt::CaseInsensitive) == 0) {
            QJsonObject headers;
            headers[QStringLiteral("Host")] = hostEff;
            xo[QStringLiteral("headers")] = headers;
        }

        const QString methodEff =
                xhttp.uplinkMethod.isEmpty() ? QString::fromLatin1(px::defaultXhttpUplinkMethod) : xhttp.uplinkMethod;
        xo[QStringLiteral("uplinkHTTPMethod")] = methodEff.toUpper();

        xo[QStringLiteral("noGRPCHeader")] = xhttp.disableGrpc;
        xo[QStringLiteral("noSSEHeader")] = xhttp.disableSse;

        const QString sessPl = normalizeSessionSeqPlacement(xhttp.sessionPlacement);
        if (!sessPl.isEmpty())
            xo[QStringLiteral("sessionPlacement")] = sessPl;
        const QString seqPl = normalizeSessionSeqPlacement(xhttp.seqPlacement);
        if (!seqPl.isEmpty())
            xo[QStringLiteral("seqPlacement")] = seqPl;
        if (!xhttp.sessionKey.isEmpty())
            xo[QStringLiteral("sessionKey")] = xhttp.sessionKey;
        if (!xhttp.seqKey.isEmpty())
            xo[QStringLiteral("seqKey")] = xhttp.seqKey;

        xo[QStringLiteral("uplinkDataPlacement")] = normalizeUplinkDataPlacement(xhttp.uplinkDataPlacement);
        if (!xhttp.uplinkDataKey.isEmpty())
            xo[QStringLiteral("uplinkDataKey")] = xhttp.uplinkDataKey;

        const QString ucs = xhttp.uplinkChunkSize.isEmpty() ? QString::fromLatin1(px::defaultXhttpUplinkChunkSize)
                                                            : xhttp.uplinkChunkSize;
        if (!ucs.isEmpty() && ucs != QLatin1String("0")) {
            const int v = ucs.toInt();
            QJsonObject chunkR;
            chunkR[QStringLiteral("from")] = v;
            chunkR[QStringLiteral("to")] = v;
            xo[QStringLiteral("uplinkChunkSize")] = chunkR;
        }

        if (!xhttp.scMaxBufferedPosts.isEmpty())
            xo[QStringLiteral("scMaxBufferedPosts")] = xhttp.scMaxBufferedPosts.toLongLong();

        putIntRangeIfAny(xo, "scMaxEachPostBytes", xhttp.scMaxEachPostBytesMin, xhttp.scMaxEachPostBytesMax,
                         px::defaultXhttpScMaxEachPostBytesMin, px::defaultXhttpScMaxEachPostBytesMax);
        putIntRangeIfAny(xo, "scMinPostsIntervalMs", xhttp.scMinPostsIntervalMsMin, xhttp.scMinPostsIntervalMsMax,
                         px::defaultXhttpScMinPostsIntervalMsMin, px::defaultXhttpScMinPostsIntervalMsMax);
        putIntRangeIfAny(xo, "scStreamUpServerSecs", xhttp.scStreamUpServerSecsMin, xhttp.scStreamUpServerSecsMax,
                         px::defaultXhttpScStreamUpServerSecsMin, px::defaultXhttpScStreamUpServerSecsMax);

        const auto &pad = xhttp.xPadding;
        xo[QStringLiteral("xPaddingObfsMode")] = pad.obfsMode;
        if (pad.obfsMode) {
            if (!pad.bytesMin.isEmpty() || !pad.bytesMax.isEmpty()) {
                QJsonObject br;
                br[QStringLiteral("from")] = pad.bytesMin.isEmpty() ? 1 : pad.bytesMin.toInt();
                br[QStringLiteral("to")] = pad.bytesMax.isEmpty() ? (pad.bytesMin.isEmpty() ? 256 : pad.bytesMin.toInt())
                                                                  : pad.bytesMax.toInt();
                xo[QStringLiteral("xPaddingBytes")] = br;
            }
            xo[QStringLiteral("xPaddingKey")] = pad.key.isEmpty() ? QStringLiteral("x_padding") : pad.key;
            xo[QStringLiteral("xPaddingHeader")] = pad.header.isEmpty() ? QStringLiteral("X-Padding") : pad.header;
            xo[QStringLiteral("xPaddingPlacement")] = normalizeXPaddingPlacement(
                    pad.placement.isEmpty() ? QString::fromLatin1(px::defaultXPaddingPlacement) : pad.placement);
            xo[QStringLiteral("xPaddingMethod")] = normalizeXPaddingMethod(
                    pad.method.isEmpty() ? QString::fromLatin1(px::defaultXPaddingMethod) : pad.method);
        }

        // xmux: Xray has no "enabled" flag; omit object when UI disables multiplex tuning.
        if (xhttp.xmux.enabled) {
            QJsonObject mux;
            auto addMuxRange = [&](const char *key, const QString &a, const QString &b) {
                if (a.isEmpty() && b.isEmpty())
                    return;
                QJsonObject r;
                r[QStringLiteral("from")] = a.isEmpty() ? 0 : a.toInt();
                r[QStringLiteral("to")] = b.isEmpty() ? 0 : b.toInt();
                mux[QString::fromUtf8(key)] = r;
            };
            addMuxRange("maxConcurrency", xhttp.xmux.maxConcurrencyMin, xhttp.xmux.maxConcurrencyMax);
            addMuxRange("maxConnections", xhttp.xmux.maxConnectionsMin, xhttp.xmux.maxConnectionsMax);
            addMuxRange("cMaxReuseTimes", xhttp.xmux.cMaxReuseTimesMin, xhttp.xmux.cMaxReuseTimesMax);
            addMuxRange("hMaxRequestTimes", xhttp.xmux.hMaxRequestTimesMin, xhttp.xmux.hMaxRequestTimesMax);
            addMuxRange("hMaxReusableSecs", xhttp.xmux.hMaxReusableSecsMin, xhttp.xmux.hMaxReusableSecsMax);
            if (!xhttp.xmux.hKeepAlivePeriod.isEmpty())
                mux[QStringLiteral("hKeepAlivePeriod")] = xhttp.xmux.hKeepAlivePeriod.toLongLong();
            if (!mux.isEmpty())
                xo[QStringLiteral("xmux")] = mux;
        }

        streamSettings[QStringLiteral("xhttpSettings")] = xo;
    }

    if (srv.transport == QLatin1String("mkcp")) {
        QJsonObject kcpObj;
        const QString ttiEff = mkcp.tti.isEmpty() ? QString::fromLatin1(px::defaultMkcpTti) : mkcp.tti;
        const QString upEff = mkcp.uplinkCapacity.isEmpty() ? QString::fromLatin1(px::defaultMkcpUplinkCapacity)
                                                            : mkcp.uplinkCapacity;
        const QString downEff = mkcp.downlinkCapacity.isEmpty() ? QString::fromLatin1(px::defaultMkcpDownlinkCapacity)
                                                                : mkcp.downlinkCapacity;
        const QString rbufEff = mkcp.readBufferSize.isEmpty() ? QString::fromLatin1(px::defaultMkcpReadBufferSize)
                                                              : mkcp.readBufferSize;
        const QString wbufEff = mkcp.writeBufferSize.isEmpty() ? QString::fromLatin1(px::defaultMkcpWriteBufferSize)
                                                               : mkcp.writeBufferSize;
        kcpObj[QStringLiteral("tti")] = ttiEff.toInt();
        kcpObj[QStringLiteral("uplinkCapacity")] = upEff.toInt();
        kcpObj[QStringLiteral("downlinkCapacity")] = downEff.toInt();
        kcpObj[QStringLiteral("readBufferSize")] = rbufEff.toInt();
        kcpObj[QStringLiteral("writeBufferSize")] = wbufEff.toInt();
        kcpObj[QStringLiteral("congestion")] = mkcp.congestion;
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
    if (const auto *xrayCfg = containerConfig.protocolConfig.as<XrayProtocolConfig>()) {
        serverConfig = &xrayCfg->serverConfig;
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

    return buildClientProtocolConfig(credentials, container, srv, xrayClientId, errorCode);
}