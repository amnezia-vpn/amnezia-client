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
#include "ui/models/protocols/awgConfigModel.h"

using namespace amnezia;
using namespace ProtocolUtils;

AwgInstaller::AwgInstaller(QObject *parent)
    : InstallerBase(parent)
{
}

QJsonObject AwgInstaller::generateConfig(DockerContainer container, int port, TransportProto transportProto)
{
    QJsonObject config = createBaseConfig(container, port, transportProto);
    
    auto mainProto = ContainerUtils::defaultProtocol(container);
    QJsonObject containerConfig = config.value(ProtocolUtils::protoToString(mainProto)).toObject();
    
    bool isAwg2 = (container == DockerContainer::Awg2);
    generateAwgParameters(containerConfig, isAwg2);
    
    if (isAwg2) {
        containerConfig[config_key::protocolVersion] = "2";
    }
    
    config.insert(ProtocolUtils::protoToString(mainProto), containerConfig);
    
    return config;
}

void AwgInstaller::generateAwgParameters(QJsonObject &containerConfig, bool isAwg2)
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

    while (usedValues.contains(s2) || s1 + AwgConstant::messageInitiationSize == s2 + AwgConstant::messageResponseSize) {
        s2 = QRandomGenerator::global()->bounded(15, 150);
    }
    usedValues.insert(s2);

    while (usedValues.contains(s3) || s1 + AwgConstant::messageInitiationSize == s3 + AwgConstant::messageCookieReplySize
           || s2 + AwgConstant::messageResponseSize == s3 + AwgConstant::messageCookieReplySize) {
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

    containerConfig[config_key::junkPacketCount] = junkPacketCount;
    containerConfig[config_key::junkPacketMinSize] = junkPacketMinSize;
    containerConfig[config_key::junkPacketMaxSize] = junkPacketMaxSize;
    containerConfig[config_key::initPacketJunkSize] = initPacketJunkSize;
    containerConfig[config_key::responsePacketJunkSize] = responsePacketJunkSize;
    containerConfig[config_key::initPacketMagicHeader] = initPacketMagicHeader;
    containerConfig[config_key::responsePacketMagicHeader] = responsePacketMagicHeader;
    containerConfig[config_key::underloadPacketMagicHeader] = underloadPacketMagicHeader;
    containerConfig[config_key::transportPacketMagicHeader] = transportPacketMagicHeader;

    containerConfig[config_key::cookieReplyPacketJunkSize] = cookieReplyPacketJunkSize;
    containerConfig[config_key::transportPacketJunkSize] = transportPacketJunkSize;

    containerConfig[config_key::specialJunk1] = protocols::awg::defaultSpecialJunk1;
    containerConfig[config_key::specialJunk2] = protocols::awg::defaultSpecialJunk2;
    containerConfig[config_key::specialJunk3] = protocols::awg::defaultSpecialJunk3;
    containerConfig[config_key::specialJunk4] = protocols::awg::defaultSpecialJunk4;
    containerConfig[config_key::specialJunk5] = protocols::awg::defaultSpecialJunk5;
}

ErrorCode AwgInstaller::extractConfigFromContainer(DockerContainer container, const ServerCredentials &credentials,
                                                   SshSession* sshSession, QJsonObject &config)
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

    auto mainProto = ContainerUtils::defaultProtocol(container);
    QJsonObject containerConfig = config.value(ProtocolUtils::protoToString(mainProto)).toObject();
    
    containerConfig[config_key::subnet_address] = serverConfigMap.value("Address").remove("/24");
    containerConfig[config_key::junkPacketCount] = serverConfigMap.value(config_key::junkPacketCount);
    containerConfig[config_key::junkPacketMinSize] = serverConfigMap.value(config_key::junkPacketMinSize);
    containerConfig[config_key::junkPacketMaxSize] = serverConfigMap.value(config_key::junkPacketMaxSize);
    containerConfig[config_key::initPacketJunkSize] = serverConfigMap.value(config_key::initPacketJunkSize);
    containerConfig[config_key::responsePacketJunkSize] = serverConfigMap.value(config_key::responsePacketJunkSize);
    containerConfig[config_key::initPacketMagicHeader] = serverConfigMap.value(config_key::initPacketMagicHeader);
    containerConfig[config_key::responsePacketMagicHeader] = serverConfigMap.value(config_key::responsePacketMagicHeader);
    containerConfig[config_key::underloadPacketMagicHeader] = serverConfigMap.value(config_key::underloadPacketMagicHeader);
    containerConfig[config_key::transportPacketMagicHeader] = serverConfigMap.value(config_key::transportPacketMagicHeader);

    // hack to parse i1-i5 from commented lines in server config
    containerConfig[config_key::specialJunk1] = serverConfigMap.value(QString("# ") + config_key::specialJunk1);
    containerConfig[config_key::specialJunk2] = serverConfigMap.value(QString("# ") + config_key::specialJunk2);
    containerConfig[config_key::specialJunk3] = serverConfigMap.value(QString("# ") + config_key::specialJunk3);
    containerConfig[config_key::specialJunk4] = serverConfigMap.value(QString("# ") + config_key::specialJunk4);
    containerConfig[config_key::specialJunk5] = serverConfigMap.value(QString("# ") + config_key::specialJunk5);

    // AWG 2.0 specific fields
    if (container == DockerContainer::Awg2) {
        containerConfig[config_key::protocolVersion] = "2";
        containerConfig[config_key::cookieReplyPacketJunkSize] = serverConfigMap.value(config_key::cookieReplyPacketJunkSize);
        containerConfig[config_key::transportPacketJunkSize] = serverConfigMap.value(config_key::transportPacketJunkSize);
    }

    config.insert(ProtocolUtils::protoToString(mainProto), containerConfig);
    
    return ErrorCode::NoError;
}

