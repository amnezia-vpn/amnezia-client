#include "awgInstaller.h"

#include <QPair>
#include <QRandomGenerator>
#include <QSet>
#include <QStringList>
#include <QVector>

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
    
    bool isAwg2 = (container == DockerContainer::Awg2);
    
    if (auto* awgConfig = config.getAwgProtocolConfig()) {
        generateAwgParameters(awgConfig->serverConfig, isAwg2);
        
        if (isAwg2) {
            awgConfig->serverConfig.protocolVersion = "2";
        }
    }
    
    return config;
}

void AwgInstaller::generateAwgParameters(AwgServerConfig &serverConfig, bool isAwg2)
{
    QString junkPacketCount = QString::number(QRandomGenerator::global()->bounded(4, 7));
    QString junkPacketMinSize = QString::number(10);
    QString junkPacketMaxSize = QString::number(50);

    int s1 = QRandomGenerator::global()->bounded(15, 150);
    int s2 = QRandomGenerator::global()->bounded(15, 150);
    int s3 = QRandomGenerator::global()->bounded(0, 64);
    int s4 = QRandomGenerator::global()->bounded(0, 20);

    // Ensure all values are unique and don't create equal packet sizes
    QSet<int> usedValues;
    usedValues.insert(s1);

    while (usedValues.contains(s2) || s1 + amnezia::AwgConstant::messageInitiationSize == s2 + amnezia::AwgConstant::messageResponseSize) {
        s2 = QRandomGenerator::global()->bounded(15, 150);
    }
    usedValues.insert(s2);

    while (usedValues.contains(s3) || s1 + amnezia::AwgConstant::messageInitiationSize == s3 + amnezia::AwgConstant::messageCookieReplySize
           || s2 + amnezia::AwgConstant::messageResponseSize == s3 + amnezia::AwgConstant::messageCookieReplySize) {
        s3 = QRandomGenerator::global()->bounded(0, 64);
    }
    usedValues.insert(s3);

    while (usedValues.contains(s4)) {
        s4 = QRandomGenerator::global()->bounded(0, 20);
    }

    QString initPacketJunkSize = QString::number(s1);
    QString responsePacketJunkSize = QString::number(s2);
    QString cookieReplyPacketJunkSize = QString::number(s3);
    QString transportPacketJunkSize = QString::number(s4);

    QString initPacketMagicHeader;
    QString responsePacketMagicHeader;
    QString underloadPacketMagicHeader;
    QString transportPacketMagicHeader;

    if (isAwg2) {
        // AWG 2.0: use range format for magic headers
        QVector<QPair<QString, QString>> headersValue;
        int min = 5;
        auto max = (std::numeric_limits<qint32>::max)();
        while (headersValue.size() != 4) {
            auto first = QRandomGenerator::global()->bounded(min, max);
            auto second = QRandomGenerator::global()->bounded(first, max);
            min = second;
            headersValue.push_back(QPair<QString, QString>(QString::number(first), QString::number(second)));
        }

        initPacketMagicHeader = headersValue.at(0).first + "-" + headersValue.at(0).second;
        responsePacketMagicHeader = headersValue.at(1).first + "-" + headersValue.at(1).second;
        underloadPacketMagicHeader = headersValue.at(2).first + "-" + headersValue.at(2).second;
        transportPacketMagicHeader = headersValue.at(3).first + "-" + headersValue.at(3).second;
    } else {
        // AWG legacy: use single values for magic headers
        QSet<QString> headersValue;
        while (headersValue.size() != 4) {
            auto max = (std::numeric_limits<qint32>::max)();
            headersValue.insert(QString::number(QRandomGenerator::global()->bounded(5, max)));
        }

        auto headersValueList = headersValue.values();
        initPacketMagicHeader = headersValueList.at(0);
        responsePacketMagicHeader = headersValueList.at(1);
        underloadPacketMagicHeader = headersValueList.at(2);
        transportPacketMagicHeader = headersValueList.at(3);
    }

    serverConfig.junkPacketCount = junkPacketCount;
    serverConfig.junkPacketMinSize = junkPacketMinSize;
    serverConfig.junkPacketMaxSize = junkPacketMaxSize;
    serverConfig.initPacketJunkSize = initPacketJunkSize;
    serverConfig.responsePacketJunkSize = responsePacketJunkSize;
    serverConfig.initPacketMagicHeader = initPacketMagicHeader;
    serverConfig.responsePacketMagicHeader = responsePacketMagicHeader;
    serverConfig.underloadPacketMagicHeader = underloadPacketMagicHeader;
    serverConfig.transportPacketMagicHeader = transportPacketMagicHeader;

    serverConfig.cookieReplyPacketJunkSize = cookieReplyPacketJunkSize;
    serverConfig.transportPacketJunkSize = transportPacketJunkSize;

    serverConfig.specialJunk1 = protocols::awg::defaultSpecialJunk1;
    serverConfig.specialJunk2 = protocols::awg::defaultSpecialJunk2;
    serverConfig.specialJunk3 = protocols::awg::defaultSpecialJunk3;
    serverConfig.specialJunk4 = protocols::awg::defaultSpecialJunk4;
    serverConfig.specialJunk5 = protocols::awg::defaultSpecialJunk5;
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

        // AWG 2.0 specific fields
        if (container == DockerContainer::Awg2) {
            awgConfig->serverConfig.protocolVersion = "2";
            awgConfig->serverConfig.cookieReplyPacketJunkSize = serverConfigMap.value(configKey::cookieReplyPacketJunkSize);
            awgConfig->serverConfig.transportPacketJunkSize = serverConfigMap.value(configKey::transportPacketJunkSize);
        }
    }
    
    return ErrorCode::NoError;
}

