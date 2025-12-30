#include "awgInstaller.h"

#include <QRandomGenerator>
#include <QSet>
#include <QStringList>

#include "containers/containers_defs.h"
#include "protocols/protocols_defs.h"
#include "core/controllers/serverController.h"
#include "utilities.h"
#include "ui/models/protocols/awgConfigModel.h"

using namespace amnezia;

AwgInstaller::AwgInstaller(QObject *parent)
    : InstallerBase(parent)
{
}

QJsonObject AwgInstaller::generateConfig(DockerContainer container, int port, TransportProto transportProto)
{
    QJsonObject config = createBaseConfig(container, port, transportProto);
    
    auto mainProto = ContainerProps::defaultProtocol(container);
    QJsonObject containerConfig = config.value(ProtocolProps::protoToString(mainProto)).toObject();
    
    generateAwgParameters(containerConfig);
    
    config.insert(ProtocolProps::protoToString(mainProto), containerConfig);
    
    return config;
}

void AwgInstaller::generateAwgParameters(QJsonObject &containerConfig)
{
    QString junkPacketCount = QString::number(QRandomGenerator::global()->bounded(4, 7));
    QString junkPacketMinSize = QString::number(10);
    QString junkPacketMaxSize = QString::number(50);

    int s1 = QRandomGenerator::global()->bounded(15, 150);
    int s2 = QRandomGenerator::global()->bounded(15, 150);
    // int s3 = QRandomGenerator::global()->bounded(15, 150);
    // int s4 = QRandomGenerator::global()->bounded(15, 150);

    // Ensure all values are unique and don't create equal packet sizes
    QSet<int> usedValues;
    usedValues.insert(s1);

    while (usedValues.contains(s2) || s1 + AwgConstant::messageInitiationSize == s2 + AwgConstant::messageResponseSize) {
        s2 = QRandomGenerator::global()->bounded(15, 150);
    }
    usedValues.insert(s2);

    // while (usedValues.contains(s3)
    //        || s1 + AwgConstant::messageInitiationSize == s3 + AwgConstant::messageCookieReplySize
    //        || s2 + AwgConstant::messageResponseSize == s3 + AwgConstant::messageCookieReplySize) {
    //     s3 = QRandomGenerator::global()->bounded(15, 150);
    // }
    // usedValues.insert(s3);

    // while (usedValues.contains(s4)
    //        || s1 + AwgConstant::messageInitiationSize == s4 + AwgConstant::messageTransportSize
    //        || s2 + AwgConstant::messageResponseSize == s4 + AwgConstant::messageTransportSize
    //        || s3 + AwgConstant::messageCookieReplySize == s4 + AwgConstant::messageTransportSize) {
    //     s4 = QRandomGenerator::global()->bounded(15, 150);
    // }

    QString initPacketJunkSize = QString::number(s1);
    QString responsePacketJunkSize = QString::number(s2);
    // QString cookieReplyPacketJunkSize = QString::number(s3);
    // QString transportPacketJunkSize = QString::number(s4);

    QSet<QString> headersValue;
    while (headersValue.size() != 4) {
        auto max = (std::numeric_limits<qint32>::max)();
        headersValue.insert(QString::number(QRandomGenerator::global()->bounded(5, max)));
    }

    auto headersValueList = headersValue.values();

    QString initPacketMagicHeader = headersValueList.at(0);
    QString responsePacketMagicHeader = headersValueList.at(1);
    QString underloadPacketMagicHeader = headersValueList.at(2);
    QString transportPacketMagicHeader = headersValueList.at(3);

    containerConfig[config_key::junkPacketCount] = junkPacketCount;
    containerConfig[config_key::junkPacketMinSize] = junkPacketMinSize;
    containerConfig[config_key::junkPacketMaxSize] = junkPacketMaxSize;
    containerConfig[config_key::initPacketJunkSize] = initPacketJunkSize;
    containerConfig[config_key::responsePacketJunkSize] = responsePacketJunkSize;
    containerConfig[config_key::initPacketMagicHeader] = initPacketMagicHeader;
    containerConfig[config_key::responsePacketMagicHeader] = responsePacketMagicHeader;
    containerConfig[config_key::underloadPacketMagicHeader] = underloadPacketMagicHeader;
    containerConfig[config_key::transportPacketMagicHeader] = transportPacketMagicHeader;

    // TODO:
    // containerConfig[config_key::cookieReplyPacketJunkSize] = cookieReplyPacketJunkSize;
    // containerConfig[config_key::transportPacketJunkSize] = transportPacketJunkSize;

    // containerConfig[config_key::specialJunk1] = specialJunk1;
    // containerConfig[config_key::specialJunk2] = specialJunk2;
    // containerConfig[config_key::specialJunk3] = specialJunk3;
    // containerConfig[config_key::specialJunk4] = specialJunk4;
    // containerConfig[config_key::specialJunk5] = specialJunk5;
    // containerConfig[config_key::controlledJunk1] = controlledJunk1;
    // containerConfig[config_key::controlledJunk2] = controlledJunk2;
    // containerConfig[config_key::controlledJunk3] = controlledJunk3;
    // containerConfig[config_key::specialHandshakeTimeout] = specialHandshakeTimeout;
}

ErrorCode AwgInstaller::extractConfigFromContainer(DockerContainer container, const ServerCredentials &credentials,
                                                   ServerController* serverController, QJsonObject &config)
{
    ErrorCode errorCode = ErrorCode::NoError;
    QString serverConfig = serverController->getTextFileFromContainer(container, credentials,
                                                                      protocols::awg::serverConfigPath, errorCode);
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

    auto mainProto = ContainerProps::defaultProtocol(container);
    QJsonObject containerConfig = config.value(ProtocolProps::protoToString(mainProto)).toObject();
    
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

    // containerConfig[config_key::cookieReplyPacketJunkSize] = serverConfigMap.value(config_key::cookieReplyPacketJunkSize);
    // containerConfig[config_key::transportPacketJunkSize] = serverConfigMap.value(config_key::transportPacketJunkSize);

    // containerConfig[config_key::specialJunk1] = serverConfigMap.value(config_key::specialJunk1);
    // containerConfig[config_key::specialJunk2] = serverConfigMap.value(config_key::specialJunk2);
    // containerConfig[config_key::specialJunk3] = serverConfigMap.value(config_key::specialJunk3);
    // containerConfig[config_key::specialJunk4] = serverConfigMap.value(config_key::specialJunk4);
    // containerConfig[config_key::specialJunk5] = serverConfigMap.value(config_key::specialJunk5);
    // containerConfig[config_key::controlledJunk1] = serverConfigMap.value(config_key::controlledJunk1);
    // containerConfig[config_key::controlledJunk2] = serverConfigMap.value(config_key::controlledJunk2);
    // containerConfig[config_key::controlledJunk3] = serverConfigMap.value(config_key::controlledJunk3);
    // containerConfig[config_key::specialHandshakeTimeout] = serverConfigMap.value(config_key::specialHandshakeTimeout);

    config.insert(ProtocolProps::protoToString(mainProto), containerConfig);
    
    return ErrorCode::NoError;
}

