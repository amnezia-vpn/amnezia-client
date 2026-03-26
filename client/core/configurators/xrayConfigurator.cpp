#include "xrayConfigurator.h"

#include "logger.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>

#include "core/models/containerConfig.h"
#include "core/models/protocols/xrayProtocolConfig.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/utils/selfhosted/scriptsRegistry.h"
#include "core/utils/selfhosted/sshSession.h"

namespace {
Logger logger("XrayConfigurator");
}

XrayConfigurator::XrayConfigurator(SshSession* sshSession, QObject *parent)
    : ConfiguratorBase(sshSession, parent)
{
}

QString XrayConfigurator::prepareServerConfig(const ServerCredentials &credentials, DockerContainer container,
                                               const ContainerConfig &containerConfig,
                                               const DnsSettings &dnsSettings,
                                               ErrorCode &errorCode)
{
    // Generate new UUID for client
    QString clientId = QUuid::createUuid().toString(QUuid::WithoutBraces);

    // Get flow value from settings (default xtls-rprx-vision)
    QString flowValue = "xtls-rprx-vision";
    if (const auto *xrayCfg = containerConfig.protocolConfig.as<XrayProtocolConfig>()) {
        if (!xrayCfg->serverConfig.flow.isEmpty()) {
            flowValue = xrayCfg->serverConfig.flow;
        }
    }

    // Get current server config
    QString currentConfig = m_sshSession->getTextFileFromContainer(
        container, credentials, amnezia::protocols::xray::serverConfigPath, errorCode);

    if (errorCode != ErrorCode::NoError) {
        logger.error() << "Failed to get server config file";
        return "";
    }

    // Parse current config as JSON
    QJsonDocument doc = QJsonDocument::fromJson(currentConfig.toUtf8());
    if (doc.isNull() || !doc.isObject()) {
        logger.error() << "Failed to parse server config JSON";
        errorCode = ErrorCode::InternalError;
        return "";
    }

    QJsonObject serverConfig = doc.object();

    // Validate server config structure
    if (!serverConfig.contains(amnezia::protocols::xray::inbounds)) {
        logger.error() << "Server config missing 'inbounds' field";
        errorCode = ErrorCode::InternalError;
        return "";
    }

    QJsonArray inbounds = serverConfig[amnezia::protocols::xray::inbounds].toArray();
    if (inbounds.isEmpty()) {
        logger.error() << "Server config has empty 'inbounds' array";
        errorCode = ErrorCode::InternalError;
        return "";
    }

    QJsonObject inbound = inbounds[0].toObject();
    if (!inbound.contains(amnezia::protocols::xray::settings)) {
        logger.error() << "Inbound missing 'settings' field";
        errorCode = ErrorCode::InternalError;
        return "";
    }

    QJsonObject settings = inbound[amnezia::protocols::xray::settings].toObject();
    if (!settings.contains(amnezia::protocols::xray::clients)) {
        logger.error() << "Settings missing 'clients' field";
        errorCode = ErrorCode::InternalError;
        return "";
    }

    QJsonArray clients = settings[amnezia::protocols::xray::clients].toArray();

    // Create configuration for new client
    QJsonObject clientConfig {
        {amnezia::protocols::xray::id, clientId},
    };
    clientConfig[amnezia::protocols::xray::id] = clientId;
    if (!flowValue.isEmpty()) {
        clientConfig[amnezia::protocols::xray::flow] = flowValue;
    }

    clients.append(clientConfig);

    // Update config
    settings[amnezia::protocols::xray::clients] = clients;
    inbound[amnezia::protocols::xray::settings] = settings;
    inbounds[0] = inbound;
    serverConfig[amnezia::protocols::xray::inbounds] = inbounds;

    // Save updated config to server
    QString updatedConfig = QJsonDocument(serverConfig).toJson();
    errorCode = m_sshSession->uploadTextFileToContainer(
        container,
        credentials,
        updatedConfig,
        amnezia::protocols::xray::serverConfigPath,
        libssh::ScpOverwriteMode::ScpOverwriteExisting
    );
    if (errorCode != ErrorCode::NoError) {
        logger.error() << "Failed to upload updated config";
        return "";
    }

    // Restart container
    QString restartScript = QString("sudo docker restart $CONTAINER_NAME");
    errorCode = m_sshSession->runScript(
        credentials,
        m_sshSession->replaceVars(restartScript, amnezia::genBaseVars(credentials, container, dnsSettings.primaryDns, dnsSettings.secondaryDns))
    );

    if (errorCode != ErrorCode::NoError) {
        logger.error() << "Failed to restart container";
        return "";
    }

    return clientId;
}

QJsonObject XrayConfigurator::buildStreamSettings(const XrayServerConfig &srv, const QString &clientId) const
{
    QJsonObject streamSettings;
    const auto &xhttp = srv.xhttp;
    const auto &mkcp = srv.mkcp;

    // network
    QString networkValue = "tcp";
    if (srv.transport == "xhttp")
    {
        networkValue = "xhttp";
    }
    else if (srv.transport == "mkcp")
    {
        networkValue = "kcp";
    }
    streamSettings[amnezia::protocols::xray::network] = networkValue;

    // security
    streamSettings[amnezia::protocols::xray::security] = srv.security;

    // TLS settings
    if (srv.security == "tls") {
        QJsonObject tlsSettings;
        if (!srv.sni.isEmpty()) {
            tlsSettings[amnezia::protocols::xray::serverName] = srv.sni;
        }
        if (!srv.alpn.isEmpty()) {
            QJsonArray alpnArray;
            // alpn may be comma-separated: "HTTP/2,HTTP/1.1"
            for (const QString &a : srv.alpn.split(",")) {
                alpnArray.append(a.trimmed());
            }
            tlsSettings["alpn"] = alpnArray;
        }
        if (!srv.fingerprint.isEmpty()) {
            tlsSettings[amnezia::protocols::xray::fingerprint] = srv.fingerprint;
        }
        streamSettings["tlsSettings"] = tlsSettings;
    }

    // Reality settings
    if (srv.security == "reality") {
        QJsonObject realSettings;
        if (!srv.fingerprint.isEmpty())
        {
            realSettings[amnezia::protocols::xray::fingerprint] = srv.fingerprint;
        }
        if (!srv.sni.isEmpty())
        {
            realSettings[amnezia::protocols::xray::serverName] = srv.sni;
        }
        // publicKey and shortId are filled in createConfig after fetching from server
        streamSettings[amnezia::protocols::xray::realitySettings] = realSettings;
    }

    // XHTTP transport settings
    if (srv.transport == "xhttp") {
        QJsonObject xhttpObj;

        if (!xhttp.host.isEmpty())
        {
            xhttpObj["host"] = xhttp.host;
        }
        if (!xhttp.path.isEmpty())
        {
            xhttpObj["path"] = xhttp.path;
        }
        if (!xhttp.mode.isEmpty())
        {
            xhttpObj["mode"] = xhttp.mode;
        }

        // headers
        if (xhttp.headersTemplate == "HTTP") {
            QJsonObject headers;
            headers["Host"] = xhttp.host;
            xhttpObj["headers"] = headers;
        }

        if (!xhttp.uplinkMethod.isEmpty())
        {
            xhttpObj["method"] = xhttp.uplinkMethod;
        }
        if (xhttp.disableGrpc)
        {
            xhttpObj["noGRPCHeader"] = true;
        }
        if (xhttp.disableSse)
        {
            xhttpObj["noSSEHeader"] = true;
        }

        // Session & Sequence
        if (!xhttp.sessionPlacement.isEmpty() && xhttp.sessionPlacement != "None")
        {
            xhttpObj["scSessionPlacement"] = xhttp.sessionPlacement;
        }
        if (!xhttp.seqPlacement.isEmpty() && xhttp.seqPlacement != "None")
        {
            xhttpObj["scSeqPlacement"] = xhttp.seqPlacement;
        }
        if (!xhttp.uplinkDataPlacement.isEmpty())
        {
            xhttpObj["scUplinkDataPlacement"] = xhttp.uplinkDataPlacement;
        }

        // Traffic shaping
        if (!xhttp.uplinkChunkSize.isEmpty() && xhttp.uplinkChunkSize != "0")
            xhttpObj["xhttpUplinkChunkSize"] = xhttp.uplinkChunkSize.toInt();
        if (!xhttp.scMaxBufferedPosts.isEmpty())
            xhttpObj["scMaxBufferedPosts"] = xhttp.scMaxBufferedPosts.toInt();

        // scMaxEachPostBytes range
        if (!xhttp.scMaxEachPostBytesMin.isEmpty() || !xhttp.scMaxEachPostBytesMax.isEmpty()) {
            QJsonObject range;
            range["from"] = xhttp.scMaxEachPostBytesMin.toInt();
            range["to"] = xhttp.scMaxEachPostBytesMax.toInt();
            xhttpObj["scMaxEachPostBytes"] = range;
        }

        // scMinPostsIntervalMs range
        if (!xhttp.scMinPostsIntervalMsMin.isEmpty() || !xhttp.scMinPostsIntervalMsMax.isEmpty()) {
            QJsonObject range;
            range["from"] = xhttp.scMinPostsIntervalMsMin.toInt();
            range["to"] = xhttp.scMinPostsIntervalMsMax.toInt();
            xhttpObj["scMinPostsIntervalMs"] = range;
        }

        // scStreamUpServerSecs range
        if (!xhttp.scStreamUpServerSecsMin.isEmpty() || !xhttp.scStreamUpServerSecsMax.isEmpty()) {
            QJsonObject range;
            range["from"] = xhttp.scStreamUpServerSecsMin.toInt();
            range["to"] = xhttp.scStreamUpServerSecsMax.toInt();
            xhttpObj["scStreamUpServerSecs"] = range;
        }

        // xPadding
        if (xhttp.xPadding.obfsMode) {
            QJsonObject paddingObj;
            if (!xhttp.xPadding.bytesMin.isEmpty() || !xhttp.xPadding.bytesMax.isEmpty()) {
                QJsonObject bytesRange;
                bytesRange["from"] = xhttp.xPadding.bytesMin.toInt();
                bytesRange["to"] = xhttp.xPadding.bytesMax.toInt();
                paddingObj["xPaddingBytes"] = bytesRange;
            }
            if (!xhttp.xPadding.key.isEmpty())
            {
                paddingObj["xPaddingKey"] = xhttp.xPadding.key;
            }
            if (!xhttp.xPadding.header.isEmpty())
            {
                paddingObj["xPaddingHeader"] = xhttp.xPadding.header;
            }
            if (!xhttp.xPadding.placement.isEmpty())
            {
                paddingObj["xPaddingPlacement"] = xhttp.xPadding.placement;
            }
            if (!xhttp.xPadding.method.isEmpty())
            {
                paddingObj["xPaddingMethod"] = xhttp.xPadding.method;
            }
            xhttpObj["xPadding"] = paddingObj;
        }

        // xmux
        if (xhttp.xmux.enabled) {
            QJsonObject muxObj;
            muxObj["enabled"] = true;

            auto addRange = [&](const char *key, const QString &minV, const QString &maxV) {
                if (!minV.isEmpty() || !maxV.isEmpty()) {
                    QJsonObject r;
                    r["from"] = minV.toInt();
                    r["to"] = maxV.toInt();
                    muxObj[key] = r;
                }
            };

            addRange("maxConcurrency", xhttp.xmux.maxConcurrencyMin, xhttp.xmux.maxConcurrencyMax);
            addRange("maxConnections", xhttp.xmux.maxConnectionsMin, xhttp.xmux.maxConnectionsMax);
            addRange("cMaxReuseTimes", xhttp.xmux.cMaxReuseTimesMin, xhttp.xmux.cMaxReuseTimesMax);
            addRange("hMaxRequestTimes", xhttp.xmux.hMaxRequestTimesMin, xhttp.xmux.hMaxRequestTimesMax);
            addRange("hMaxReusableSecs", xhttp.xmux.hMaxReusableSecsMin, xhttp.xmux.hMaxReusableSecsMax);

            if (!xhttp.xmux.hKeepAlivePeriod.isEmpty())
            {
                muxObj["hKeepAlivePeriod"] = xhttp.xmux.hKeepAlivePeriod.toInt();
            }

            xhttpObj["xmux"] = muxObj;
        }

        streamSettings["xhttpSettings"] = xhttpObj;
    }

    // mKCP transport settings
    if (srv.transport == "mkcp") {
        QJsonObject kcpObj;
        if (!mkcp.tti.isEmpty())
        {
            kcpObj["tti"] = mkcp.tti.toInt();
        }
        if (!mkcp.uplinkCapacity.isEmpty())
        {
            kcpObj["uplinkCapacity"] = mkcp.uplinkCapacity.toInt();
        }
        if (!mkcp.downlinkCapacity.isEmpty())
        {
            kcpObj["downlinkCapacity"] = mkcp.downlinkCapacity.toInt();
        }
        if (!mkcp.readBufferSize.isEmpty())
        {
            kcpObj["readBufferSize"] = mkcp.readBufferSize.toInt();
        }
        if (!mkcp.writeBufferSize.isEmpty())
        {
            kcpObj["writeBufferSize"] = mkcp.writeBufferSize.toInt();
        }
        kcpObj["congestion"] = mkcp.congestion;
        streamSettings["kcpSettings"] = kcpObj;
    }

    return streamSettings;
}

ProtocolConfig XrayConfigurator::createConfig(const ServerCredentials &credentials, DockerContainer container,
                                               const ContainerConfig &containerConfig,
                                               const DnsSettings &dnsSettings,
                                               ErrorCode &errorCode)
{
    const XrayServerConfig *serverConfig = nullptr;
    if (const auto *xrayCfg = containerConfig.protocolConfig.as<XrayProtocolConfig>()) {
        serverConfig = &xrayCfg->serverConfig;
    }

    if (!serverConfig) {
        logger.error() << "No XrayProtocolConfig found";
        errorCode = ErrorCode::InternalError;
        return XrayProtocolConfig {};
    }

    const XrayServerConfig &srv = *serverConfig;

    QString xrayClientId = prepareServerConfig(credentials, container, containerConfig, dnsSettings, errorCode);
    if (errorCode != ErrorCode::NoError || xrayClientId.isEmpty()) {
        logger.error() << "Failed to prepare server config";
        return XrayProtocolConfig {};
    }

    // Fetch server keys (Reality only)
    QString xrayPublicKey;
    QString xrayShortId;

    if (srv.security == "reality") {
        xrayPublicKey = m_sshSession->getTextFileFromContainer(container, credentials,
                                                               amnezia::protocols::xray::PublicKeyPath, errorCode);
        if (errorCode != ErrorCode::NoError || xrayPublicKey.isEmpty()) {
            logger.error() << "Failed to get public key";
            return XrayProtocolConfig {};
        }
        xrayPublicKey.replace("\n", "");

        xrayShortId = m_sshSession->getTextFileFromContainer(container, credentials,
                                                             amnezia::protocols::xray::shortidPath, errorCode);
        if (errorCode != ErrorCode::NoError || xrayShortId.isEmpty()) {
            logger.error() << "Failed to get short ID";
            return XrayProtocolConfig {};
        }
        xrayShortId.replace("\n", "");
    }

    // Build outbound
    QJsonObject userObj;
    userObj[amnezia::protocols::xray::id] = xrayClientId;
    userObj[amnezia::protocols::xray::encryption] = "none";
    if (!srv.flow.isEmpty()) {
        userObj[amnezia::protocols::xray::flow] = srv.flow;
    }

    QJsonObject vnextEntry;
    vnextEntry[amnezia::protocols::xray::address] = credentials.hostName;
    vnextEntry[amnezia::protocols::xray::port] = srv.port.toInt();
    vnextEntry[amnezia::protocols::xray::users] = QJsonArray { userObj };

    QJsonObject outboundSettings;
    outboundSettings[amnezia::protocols::xray::vnext] = QJsonArray { vnextEntry };

    QJsonObject outbound;
    outbound["protocol"] = "vless";
    outbound[amnezia::protocols::xray::settings] = outboundSettings;

    // Build streamSettings
    QJsonObject streamObj = buildStreamSettings(srv, xrayClientId);

    // Inject Reality keys
    if (srv.security == "reality") {
        QJsonObject rs = streamObj[amnezia::protocols::xray::realitySettings].toObject();
        rs[amnezia::protocols::xray::publicKey] = xrayPublicKey;
        rs[amnezia::protocols::xray::shortId] = xrayShortId;
        rs[amnezia::protocols::xray::spiderX] = "";
        streamObj[amnezia::protocols::xray::realitySettings] = rs;
    }

    outbound[amnezia::protocols::xray::streamSettings] = streamObj;

    // Build full client config
    QJsonObject inboundObj;
    inboundObj["listen"] = amnezia::protocols::xray::defaultLocalAddr;
    inboundObj[amnezia::protocols::xray::port] = amnezia::protocols::xray::defaultLocalProxyPort;
    inboundObj["protocol"] = "socks";
    inboundObj[amnezia::protocols::xray::settings] = QJsonObject { { "udp", true } };

    QJsonObject clientJson;
    clientJson["log"] = QJsonObject { { "loglevel", "error" } };
    clientJson[amnezia::protocols::xray::inbounds] = QJsonArray { inboundObj };
    clientJson[amnezia::protocols::xray::outbounds] = QJsonArray { outbound };

    QString config = QString::fromUtf8(QJsonDocument(clientJson).toJson(QJsonDocument::Compact));

    // Return
    XrayProtocolConfig protocolConfig;
    protocolConfig.serverConfig = srv;

    XrayClientConfig clientConfig;
    clientConfig.nativeConfig = config;
    clientConfig.localPort = QString(amnezia::protocols::xray::defaultLocalProxyPort);
    clientConfig.id = xrayClientId;

    protocolConfig.setClientConfig(clientConfig);

    return protocolConfig;
}
