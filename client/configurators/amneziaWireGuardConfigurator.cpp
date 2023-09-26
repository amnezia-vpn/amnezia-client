#include "amneziaWireGuardConfigurator.h"

#include <QJsonDocument>
#include <QJsonObject>

AmneziaWireGuardConfigurator::AmneziaWireGuardConfigurator(std::shared_ptr<Settings> settings, QObject *parent)
    : WireguardConfigurator(settings, true, parent)
{
}

QString AmneziaWireGuardConfigurator::genAmneziaWireGuardConfig(const ServerCredentials &credentials,
                                                                DockerContainer container,
                                                                const QJsonObject &containerConfig, ErrorCode *errorCode)
{
    QString config = WireguardConfigurator::genWireguardConfig(credentials, container, containerConfig, errorCode);

    QJsonObject jsonConfig = QJsonDocument::fromJson(config.toUtf8()).object();
    QJsonObject awgConfig = containerConfig.value(config_key::amneziaWireguard).toObject();

    auto junkPacketCount =
            awgConfig.value(config_key::junkPacketCount).toString(protocols::amneziawireguard::defaultJunkPacketCount);
    auto junkPacketMinSize =
            awgConfig.value(config_key::junkPacketMinSize).toString(protocols::amneziawireguard::defaultJunkPacketMinSize);
    auto junkPacketMaxSize =
            awgConfig.value(config_key::junkPacketMaxSize).toString(protocols::amneziawireguard::defaultJunkPacketMaxSize);
    auto initPacketJunkSize =
            awgConfig.value(config_key::initPacketJunkSize).toString(protocols::amneziawireguard::defaultInitPacketJunkSize);
    auto responsePacketJunkSize =
            awgConfig.value(config_key::responsePacketJunkSize).toString(protocols::amneziawireguard::defaultResponsePacketJunkSize);
    auto initPacketMagicHeader =
            awgConfig.value(config_key::initPacketMagicHeader).toString(protocols::amneziawireguard::defaultInitPacketMagicHeader);
    auto responsePacketMagicHeader =
            awgConfig.value(config_key::responsePacketMagicHeader).toString(protocols::amneziawireguard::defaultResponsePacketMagicHeader);
    auto underloadPacketMagicHeader =
            awgConfig.value(config_key::underloadPacketMagicHeader).toString(protocols::amneziawireguard::defaultUnderloadPacketMagicHeader);
    auto transportPacketMagicHeader =
            awgConfig.value(config_key::transportPacketMagicHeader).toString(protocols::amneziawireguard::defaultTransportPacketMagicHeader);

    config.replace("$JUNK_PACKET_COUNT", junkPacketCount);
    config.replace("$JUNK_PACKET_MIN_SIZE", junkPacketMinSize);
    config.replace("$JUNK_PACKET_MAX_SIZE", junkPacketMaxSize);
    config.replace("$INIT_PACKET_JUNK_SIZE", initPacketJunkSize);
    config.replace("$RESPONSE_PACKET_JUNK_SIZE", responsePacketJunkSize);
    config.replace("$INIT_PACKET_MAGIC_HEADER", initPacketMagicHeader);
    config.replace("$RESPONSE_PACKET_MAGIC_HEADER", responsePacketMagicHeader);
    config.replace("$UNDERLOAD_PACKET_MAGIC_HEADER", underloadPacketMagicHeader);
    config.replace("$TRANSPORT_PACKET_MAGIC_HEADER", transportPacketMagicHeader);

    jsonConfig[config_key::junkPacketCount] = junkPacketCount;
    jsonConfig[config_key::junkPacketMinSize] = junkPacketMinSize;
    jsonConfig[config_key::junkPacketMaxSize] = junkPacketMaxSize;
    jsonConfig[config_key::initPacketJunkSize] = initPacketJunkSize;
    jsonConfig[config_key::responsePacketJunkSize] = responsePacketJunkSize;
    jsonConfig[config_key::initPacketMagicHeader] = initPacketMagicHeader;
    jsonConfig[config_key::responsePacketMagicHeader] = responsePacketMagicHeader;
    jsonConfig[config_key::underloadPacketMagicHeader] = underloadPacketMagicHeader;
    jsonConfig[config_key::transportPacketMagicHeader] = transportPacketMagicHeader;

    return QJsonDocument(jsonConfig).toJson();
}
