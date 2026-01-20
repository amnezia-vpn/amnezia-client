#include "xrayInstaller.h"

#include <QJsonDocument>
#include <QJsonArray>

#include "containers/containers_defs.h"
#include "core/protocols/protocolsDefs.h"
#include "core/utils/selfhosted/sshSession.h"
#include "logger.h"

namespace {
    Logger logger("XrayInstaller");
}

using namespace amnezia;

XrayInstaller::XrayInstaller(QObject *parent)
    : InstallerBase(parent)
{
}

ErrorCode XrayInstaller::extractConfigFromContainer(DockerContainer container, const ServerCredentials &credentials,
                                                     SshSession* sshSession, QJsonObject &config)
{
    ErrorCode errorCode = ErrorCode::NoError;
    QString currentConfig = sshSession->getTextFileFromContainer(
            container, credentials, amnezia::protocols::xray::serverConfigPath, errorCode);

    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    QJsonDocument doc = QJsonDocument::fromJson(currentConfig.toUtf8());
    if (doc.isNull() || !doc.isObject()) {
        logger.error() << "Failed to parse server config JSON";
        return ErrorCode::InternalError;
    }
    QJsonObject serverConfig = doc.object();

    if (!serverConfig.contains("inbounds")) {
        logger.error() << "Server config missing 'inbounds' field";
        return ErrorCode::InternalError;
    }

    QJsonArray inbounds = serverConfig["inbounds"].toArray();
    if (inbounds.isEmpty()) {
        logger.error() << "Server config has empty 'inbounds' array";
        return ErrorCode::InternalError;
    }

    QJsonObject inbound = inbounds[0].toObject();
    if (!inbound.contains("streamSettings")) {
        logger.error() << "Inbound missing 'streamSettings' field";
        return ErrorCode::InternalError;
    }

    QJsonObject streamSettings = inbound["streamSettings"].toObject();
    QJsonObject realitySettings = streamSettings["realitySettings"].toObject();
    if (!realitySettings.contains("serverNames")) {
        logger.error() << "Settings missing 'serverNames' field";
        return ErrorCode::InternalError;
    }

    QString siteName = realitySettings["serverNames"][0].toString();

    auto mainProto = ContainerProps::defaultProtocol(container);
    QJsonObject containerConfig = config.value(ProtocolProps::protoToString(mainProto)).toObject();
    
    containerConfig.insert(config_key::site, siteName);

    config.insert(ProtocolProps::protoToString(mainProto), containerConfig);
    
    return ErrorCode::NoError;
}

