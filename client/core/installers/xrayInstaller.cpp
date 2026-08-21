#include "xrayInstaller.h"

#include <QJsonDocument>

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/utils/selfhosted/sshSession.h"
#include "core/configurators/xrayConfigurator.h"
#include "core/models/protocols/xrayProtocolConfig.h"
#include "logger.h"

namespace
{
    Logger logger("XrayInstaller");

    QString describeServerJsonStatus(amnezia::XrayServerJsonStatus status)
    {
        switch (status) {
        case amnezia::XrayServerJsonStatus::MissingInbounds:
            return QStringLiteral("server config missing 'inbounds' field");
        case amnezia::XrayServerJsonStatus::EmptyInbounds:
            return QStringLiteral("server config has empty 'inbounds' array");
        case amnezia::XrayServerJsonStatus::MissingStreamSettings:
            return QStringLiteral("inbound missing 'streamSettings' field");
        case amnezia::XrayServerJsonStatus::MissingSettings:
            return QStringLiteral("inbound missing 'settings' field");
        case amnezia::XrayServerJsonStatus::Ok:
            break;
        }
        return QStringLiteral("ok");
    }
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
    namespace px = amnezia::protocols::xray;

    ErrorCode errorCode = ErrorCode::NoError;

    QString currentConfig = sshSession->getTextFileFromContainer(
            container, credentials, px::serverConfigPath, errorCode);

    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    QJsonDocument doc = QJsonDocument::fromJson(currentConfig.toUtf8());
    if (doc.isNull() || !doc.isObject()) {
        logger.error() << "Failed to parse server config JSON";
        return ErrorCode::InternalError;
    }
    auto *xrayConfig = config.getXrayProtocolConfig();
    if (!xrayConfig) {
        logger.error() << "No XrayProtocolConfig in ContainerConfig";
        return ErrorCode::InternalError;
    }

    XrayClientTemplate &tpl = xrayConfig->clientTemplate;
    const amnezia::XrayServerJsonStatus status =
            XrayServerConfig::fromServerInboundJson(doc.object(), xrayConfig->serverConfig, tpl);
    if (status != amnezia::XrayServerJsonStatus::Ok) {
        logger.error() << "Xray extractConfigFromContainer:" << describeServerJsonStatus(status);
        return ErrorCode::InternalError;
    }

    logger.info() << "Xray extractConfigFromContainer: extracted server, port=" << xrayConfig->serverConfig.port
                  << "transport=" << xrayConfig->serverConfig.transport
                  << "security=" << xrayConfig->serverConfig.security << "site=" << xrayConfig->serverConfig.site
                  << "sni=" << xrayConfig->serverConfig.sni;

    {
        XrayConfigurator configurator(sshSession);
        bool found = false;
        const XrayClientTemplate stored = configurator.readClientTemplate(credentials, container, found);
        if (found) {
            tpl = stored;
            logger.info() << "Xray extractConfigFromContainer: adopted the client template from the server,"
                          << "fingerprint=" << tpl.fingerprint << "uplinkMethod=" << tpl.uplinkMethod;
        } else {
            logger.info() << "Xray extractConfigFromContainer: no client template on the server to adopt";
        }
    }

    return ErrorCode::NoError;
}

