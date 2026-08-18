#include "awgInstaller.h"

#include <QRandomGenerator>
#include <QSet>
#include <QStringList>

#include "core/configurators/wireguardConfigurator.h"
#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/utils/selfhosted/sshSession.h"
#include "core/utils/utilities.h"
#include "core/models/protocols/awgProtocolConfig.h"

using namespace amnezia;
using namespace ProtocolUtils;

AwgInstaller::AwgInstaller(QObject *parent)
    : InstallerBase(parent)
{
}

ContainerConfig AwgInstaller::generateConfig(DockerContainer container, int port, TransportProto transportProto)
{
    ContainerConfig config = createBaseConfig(container, port, transportProto);
    if (auto* awgConfig = config.getAwgProtocolConfig()) {
        generateAwgParameters(awgConfig->serverConfig);
        awgConfig->serverConfig.protocolVersion = protocols::awg::awgV3;
    }
    return config;
}

void AwgInstaller::generateAwgParameters(AwgServerConfig &serverConfig)
{
    QString junkPacketCount = QString::number(QRandomGenerator::global()->bounded(4, 7));
    QString junkPacketMinSize = QString::number(10);
    QString junkPacketMaxSize = QString::number(50);

    int s1 = QRandomGenerator::global()->bounded(protocols::awg::junkPacketSizeMin, protocols::awg::initPacketJunkSizeMax);
    int s2 = QRandomGenerator::global()->bounded(protocols::awg::junkPacketSizeMin, protocols::awg::responsePacketJunkSizeMax);
    int s3 = QRandomGenerator::global()->bounded(protocols::awg::junkPacketSizeMin, protocols::awg::cookieReplyPacketJunkSizeMax);
    int s4 = protocols::awg::defaultTransportPacketJunkSize;

    // Ensure all values are unique and don't create equal packet sizes
    QSet<int> usedValues { s1, s4 };

    while (usedValues.contains(s2) || s1 + amnezia::AwgConstant::messageInitiationSize == s2 + amnezia::AwgConstant::messageResponseSize) {
        s2 = QRandomGenerator::global()->bounded(protocols::awg::junkPacketSizeMin, protocols::awg::responsePacketJunkSizeMax);
    }
    usedValues.insert(s2);

    while (usedValues.contains(s3) || s1 + amnezia::AwgConstant::messageInitiationSize == s3 + amnezia::AwgConstant::messageCookieReplySize
           || s2 + amnezia::AwgConstant::messageResponseSize == s3 + amnezia::AwgConstant::messageCookieReplySize) {
        s3 = QRandomGenerator::global()->bounded(protocols::awg::junkPacketSizeMin, protocols::awg::cookieReplyPacketJunkSizeMax);
    }

    serverConfig.junkPacketCount = junkPacketCount;
    serverConfig.junkPacketMinSize = junkPacketMinSize;
    serverConfig.junkPacketMaxSize = junkPacketMaxSize;
    serverConfig.initPacketJunkSize = QString::number(s1);
    serverConfig.responsePacketJunkSize = QString::number(s2);
    serverConfig.cookieReplyPacketJunkSize = QString::number(s3);
    serverConfig.transportPacketJunkSize = QString::number(s4);

    serverConfig.initPacketMagicHeader = protocols::awg::defaultInitPacketMagicHeader;
    serverConfig.responsePacketMagicHeader = protocols::awg::defaultResponsePacketMagicHeader;
    serverConfig.underloadPacketMagicHeader = protocols::awg::defaultUnderloadPacketMagicHeader;
    serverConfig.transportPacketMagicHeader = protocols::awg::defaultTransportPacketMagicHeader;

    serverConfig.headerProtectionKey = WireguardConfigurator::genClientKeys().clientPrivKey;
    serverConfig.contentPaddingAddition = protocols::awg::defaultContentPaddingAddition;
    serverConfig.rekeyAfterTime = protocols::awg::defaultRekeyAfterTime;
    serverConfig.rekeyTimeout = protocols::awg::defaultRekeyTimeout;
    serverConfig.rejectAfterTime = protocols::awg::defaultRejectAfterTime;
    serverConfig.keepaliveTimeout = protocols::awg::defaultKeepaliveTimeout;
    serverConfig.maxHandshakeAttempts = protocols::awg::defaultMaxHandshakeAttempts;
    serverConfig.randomTrailers = protocols::awg::defaultRandomTrailers;
    serverConfig.disableCookies = protocols::awg::defaultDisableCookies;
}

ErrorCode AwgInstaller::extractConfigFromContainer(DockerContainer container, const ServerCredentials &credentials,
                                                   SshSession* sshSession, ContainerConfig &config)
{
    ErrorCode errorCode = ErrorCode::NoError;
    
    // Use appropriate config path based on container type
    QString configPath = protocols::awg::serverConfigPath;
    if (container == DockerContainer::Awg) {
        configPath = protocols::awg::serverLegacyConfigPath;
    }
    
    QString serverConfig = sshSession->getTextFileFromContainer(container, credentials, configPath, errorCode);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    QMap<QString, QString> serverConfigMap;
    auto serverConfigLines = serverConfig.split("\n");
    for (auto &line : serverConfigLines) {
        auto trimmedLine = line.trimmed();
        if (trimmedLine.startsWith("[") && trimmedLine.endsWith("]")) {
            continue;
        } else {
            QStringList parts = trimmedLine.split(" = ");
            if (parts.count() == 2) {
                serverConfigMap.insert(parts[0].trimmed(), parts[1].trimmed());
            }
        }
    }

    if (auto* awgConfig = config.getAwgProtocolConfig()) {
        QString addressValue = serverConfigMap.value("Address");
        QStringList addressParts = addressValue.split("/");
        awgConfig->serverConfig.subnetAddress = addressParts.value(0);
        if (addressParts.size() > 1) {
            awgConfig->serverConfig.subnetCidr = addressParts.value(1);
        }
        awgConfig->serverConfig.junkPacketCount = serverConfigMap.value(configKey::junkPacketCount);
        awgConfig->serverConfig.junkPacketMinSize = serverConfigMap.value(configKey::junkPacketMinSize);
        awgConfig->serverConfig.junkPacketMaxSize = serverConfigMap.value(configKey::junkPacketMaxSize);
        awgConfig->serverConfig.initPacketJunkSize = serverConfigMap.value(configKey::initPacketJunkSize);
        awgConfig->serverConfig.responsePacketJunkSize = serverConfigMap.value(configKey::responsePacketJunkSize);
        awgConfig->serverConfig.initPacketMagicHeader = serverConfigMap.value(configKey::initPacketMagicHeader);
        awgConfig->serverConfig.responsePacketMagicHeader = serverConfigMap.value(configKey::responsePacketMagicHeader);
        awgConfig->serverConfig.underloadPacketMagicHeader = serverConfigMap.value(configKey::underloadPacketMagicHeader);
        awgConfig->serverConfig.transportPacketMagicHeader = serverConfigMap.value(configKey::transportPacketMagicHeader);

        // hack to parse i1-i5 from commented lines in server config
        awgConfig->serverConfig.specialJunk1 = serverConfigMap.value(QString("# ") + configKey::specialJunk1);
        awgConfig->serverConfig.specialJunk2 = serverConfigMap.value(QString("# ") + configKey::specialJunk2);
        awgConfig->serverConfig.specialJunk3 = serverConfigMap.value(QString("# ") + configKey::specialJunk3);
        awgConfig->serverConfig.specialJunk4 = serverConfigMap.value(QString("# ") + configKey::specialJunk4);
        awgConfig->serverConfig.specialJunk5 = serverConfigMap.value(QString("# ") + configKey::specialJunk5);

        awgConfig->serverConfig.cookieReplyPacketJunkSize = serverConfigMap.value(configKey::cookieReplyPacketJunkSize);
        awgConfig->serverConfig.transportPacketJunkSize = serverConfigMap.value(configKey::transportPacketJunkSize);

        awgConfig->serverConfig.headerProtectionKey = serverConfigMap.value(configKey::headerProtectionKey);
        awgConfig->serverConfig.contentPaddingAddition = serverConfigMap.value(configKey::contentPaddingAddition);
        awgConfig->serverConfig.rekeyAfterTime = serverConfigMap.value(configKey::rekeyAfterTime);
        awgConfig->serverConfig.rekeyTimeout = serverConfigMap.value(configKey::rekeyTimeout);
        awgConfig->serverConfig.rejectAfterTime = serverConfigMap.value(configKey::rejectAfterTime);
        awgConfig->serverConfig.keepaliveTimeout = serverConfigMap.value(configKey::keepaliveTimeout);
        awgConfig->serverConfig.maxHandshakeAttempts = serverConfigMap.value(configKey::maxHandshakeAttempts);
        awgConfig->serverConfig.randomTrailers = serverConfigMap.value(configKey::randomTrailers);
        awgConfig->serverConfig.disableCookies = serverConfigMap.value(configKey::disableCookies);

        awgConfig->serverConfig.protocolVersion = awgConfig->serverProtocolVersion();
    }

    return ErrorCode::NoError;
}

