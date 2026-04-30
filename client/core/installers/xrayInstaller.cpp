#include "xrayInstaller.h"

#include <QJsonDocument>
#include <QJsonArray>

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/utils/selfhosted/sshSession.h"
#include "core/models/protocols/xrayProtocolConfig.h"
#include "logger.h"

namespace {
    Logger logger("XrayInstaller");
}

using namespace amnezia;
using namespace ProtocolUtils;

XrayInstaller::XrayInstaller(QObject *parent)
    : InstallerBase(parent)
{
}

ErrorCode XrayInstaller::extractConfigFromContainer(DockerContainer container, const ServerCredentials &credentials,
                                                     SshSession* sshSession, ContainerConfig &config)
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

    if (!serverConfig.contains(protocols::xray::inbounds)) {
        logger.error() << "Server config missing 'inbounds' field";
        return ErrorCode::InternalError;
    }

    QJsonArray inbounds = serverConfig[protocols::xray::inbounds].toArray();
    if (inbounds.isEmpty()) {
        logger.error() << "Server config has empty 'inbounds' array";
        return ErrorCode::InternalError;
    }

    QJsonObject inbound = inbounds[0].toObject();
    if (!inbound.contains(protocols::xray::streamSettings)) {
        logger.error() << "Inbound missing 'streamSettings' field";
        return ErrorCode::InternalError;
    }

    QJsonObject streamSettings = inbound[protocols::xray::streamSettings].toObject();
    QJsonObject realitySettings = streamSettings[protocols::xray::realitySettings].toObject();
    if (!realitySettings.contains(protocols::xray::serverNames)) {
        logger.error() << "Settings missing 'serverNames' field";
        return ErrorCode::InternalError;
    }

    QString siteName = realitySettings[protocols::xray::serverNames][0].toString();

    if (auto* xrayConfig = config.getXrayProtocolConfig()) {
        xrayConfig->serverConfig.site = siteName;
    }
    
    return ErrorCode::NoError;
}

