#include "exportController.h"

#include <QJsonArray>
#include <QJsonDocument>

#include "configurators/configurator_base.h"
#include "core/utils/selfhosted/sshSession.h"
#include "core/utils/networkUtilities.h"
#include "core/utils/qrCodeUtils.h"
#include "core/utils/serialization/serialization.h"
#include "protocols/protocols_defs.h"

ExportController::ExportController(ServersRepository* serversRepository,
                                   AppSettingsRepository* appSettingsRepository,
                                   const std::shared_ptr<Settings> &settings,
                                   QObject *parent)
    : QObject(parent),
      m_serversRepository(serversRepository),
      m_appSettingsRepository(appSettingsRepository),
      m_settings(settings)
{
}

ExportController::ExportResult ExportController::generateFullAccessConfig(int serverIndex)
{
    ExportResult result;

    QJsonObject serverConfig = m_serversRepository->server(serverIndex);

    QJsonArray containers = serverConfig.value(config_key::containers).toArray();
    for (auto i = 0; i < containers.size(); i++) {
        auto containerConfig = containers.at(i).toObject();
        auto containerType = ContainerProps::containerFromString(containerConfig.value(config_key::container).toString());

        for (auto protocol : ContainerProps::protocolsForContainer(containerType)) {
            auto protocolConfig = containerConfig.value(ProtocolProps::protoToString(protocol)).toObject();

            protocolConfig.remove(config_key::last_config);
            containerConfig[ProtocolProps::protoToString(protocol)] = protocolConfig;
        }

        containers.replace(i, containerConfig);
    }
    serverConfig[config_key::containers] = containers;

    QByteArray compressedConfig = QJsonDocument(serverConfig).toJson();
    compressedConfig = qCompress(compressedConfig, 8);
    result.config = generateVpnUrl(compressedConfig);
    result.qrCodes = generateQrCodesFromConfig(compressedConfig);

    return result;
}

ExportController::ExportResult ExportController::generateConnectionConfig(int serverIndex, int containerIndex, const QString &clientName)
{
    ExportResult result;

    DockerContainer container = static_cast<DockerContainer>(containerIndex);
    ServerCredentials credentials = m_serversRepository->serverCredentials(serverIndex);
    QJsonObject containerConfig = m_serversRepository->containerConfig(serverIndex, container);

    containerConfig.insert(config_key::container, ContainerProps::containerToString(container));

    if (ContainerProps::containerService(container) != ServiceType::Other) {
        SshSession sshSession;
        for (Proto protocol : ContainerProps::protocolsForContainer(container)) {
            QJsonObject protocolConfig = containerConfig.value(ProtocolProps::protoToString(protocol)).toObject();

            auto configurator = ConfiguratorBase::create(protocol, m_settings, &sshSession);
            QString protocolConfigString = configurator->createConfig(credentials, container, containerConfig, result.errorCode);
            if (result.errorCode != ErrorCode::NoError) {
                return result;
            }

            protocolConfig.insert(config_key::last_config, protocolConfigString);
            containerConfig.insert(ProtocolProps::protoToString(protocol), protocolConfig);
        }
    }

    emit appendClientRequested(container, credentials, containerConfig, clientName);

    QJsonObject serverConfig = m_serversRepository->server(serverIndex);
    serverConfig.remove(config_key::userName);
    serverConfig.remove(config_key::password);
    serverConfig.remove(config_key::port);
    serverConfig.insert(config_key::containers, QJsonArray { containerConfig });
    serverConfig.insert(config_key::defaultContainer, ContainerProps::containerToString(container));

    auto dns = getDnsPair(serverIndex, m_appSettingsRepository->useAmneziaDns());
    serverConfig.insert(config_key::dns1, dns.first);
    serverConfig.insert(config_key::dns2, dns.second);

    QByteArray compressedConfig = QJsonDocument(serverConfig).toJson();
    compressedConfig = qCompress(compressedConfig, 8);
    result.config = generateVpnUrl(compressedConfig);
    result.qrCodes = generateQrCodesFromConfig(compressedConfig);

    return result;
}

ExportController::NativeConfigResult ExportController::generateNativeConfig(int serverIndex, DockerContainer container,
                                                                             const QJsonObject &containerConfig,
                                                                             const QString &clientName, Proto protocol,
                                                                             bool isApiConfig)
{
    NativeConfigResult result;

    if (ContainerProps::containerService(container) == ServiceType::Other) {
        return result;
    }

    ServerCredentials credentials = m_serversRepository->serverCredentials(serverIndex);
    auto dns = getDnsPair(serverIndex, m_appSettingsRepository->useAmneziaDns());

    QJsonObject modifiedContainerConfig = containerConfig;
    modifiedContainerConfig.insert(config_key::container, ContainerProps::containerToString(container));

    SshSession sshSession;
    auto configurator = ConfiguratorBase::create(protocol, m_settings, &sshSession);

    QString protocolConfigString = configurator->createConfig(credentials, container, modifiedContainerConfig, result.errorCode);
    if (result.errorCode != ErrorCode::NoError) {
        return result;
    }
    protocolConfigString = configurator->processConfigWithExportSettings(dns, isApiConfig, protocolConfigString);

    result.jsonNativeConfig = QJsonDocument::fromJson(protocolConfigString.toUtf8()).object();

    if (protocol == Proto::OpenVpn || protocol == Proto::WireGuard || protocol == Proto::Awg || protocol == Proto::Xray) {
        emit appendClientByConfigRequested(result.jsonNativeConfig, clientName, container, credentials);
    }
    return result;
}

ExportController::ExportResult ExportController::generateOpenVpnConfig(int serverIndex, int containerIndex,
                                                                       const QString &clientName, bool isApiConfig)
{
    ExportResult result;

    DockerContainer container = static_cast<DockerContainer>(containerIndex);
    QJsonObject containerConfig = m_serversRepository->containerConfig(serverIndex, container);

    Proto protocol;
    if (container == DockerContainer::Cloak || container == DockerContainer::ShadowSocks) {
        protocol = Proto::OpenVpn;
    } else {
        protocol = ContainerProps::defaultProtocol(container);
    }

    auto nativeResult = generateNativeConfig(serverIndex, container, containerConfig, clientName, protocol, isApiConfig);
    if (nativeResult.errorCode != ErrorCode::NoError) {
        result.errorCode = nativeResult.errorCode;
        return result;
    }

    QStringList lines = nativeResult.jsonNativeConfig.value(config_key::config).toString().replace("\r", "").split("\n");
    for (const QString &line : std::as_const(lines)) {
        result.config.append(line + "\n");
    }

    result.qrCodes = generateQrCodesFromConfig(result.config.toUtf8());
    return result;
}

ExportController::ExportResult ExportController::generateWireGuardConfig(int serverIndex, const QString &clientName, bool isApiConfig)
{
    ExportResult result;

    QJsonObject containerConfig = m_serversRepository->containerConfig(serverIndex, DockerContainer::WireGuard);

    auto nativeResult = generateNativeConfig(serverIndex, DockerContainer::WireGuard, containerConfig, clientName, Proto::WireGuard, isApiConfig);
    if (nativeResult.errorCode != ErrorCode::NoError) {
        result.errorCode = nativeResult.errorCode;
        return result;
    }

    QStringList lines = nativeResult.jsonNativeConfig.value(config_key::config).toString().replace("\r", "").split("\n");
    for (const QString &line : std::as_const(lines)) {
        result.config.append(line + "\n");
    }

    result.qrCodes << generateSingleQrCode(result.config.toUtf8());
    return result;
}

ExportController::ExportResult ExportController::generateAwgConfig(int serverIndex, int containerIndex,
                                                                   const QString &clientName, bool isApiConfig)
{
    ExportResult result;

    DockerContainer container = static_cast<DockerContainer>(containerIndex);
    QJsonObject containerConfig = m_serversRepository->containerConfig(serverIndex, container);

    auto nativeResult = generateNativeConfig(serverIndex, container, containerConfig, clientName, Proto::Awg, isApiConfig);
    if (nativeResult.errorCode != ErrorCode::NoError) {
        result.errorCode = nativeResult.errorCode;
        return result;
    }

    QStringList lines = nativeResult.jsonNativeConfig.value(config_key::config).toString().replace("\r", "").split("\n");
    for (const QString &line : std::as_const(lines)) {
        result.config.append(line + "\n");
    }

    result.qrCodes << generateSingleQrCode(result.config.toUtf8());
    return result;
}

ExportController::ExportResult ExportController::generateShadowSocksConfig(int serverIndex, int containerIndex, bool isApiConfig)
{
    ExportResult result;

    DockerContainer container = static_cast<DockerContainer>(containerIndex);
    QJsonObject containerConfig = m_serversRepository->containerConfig(serverIndex, container);

    Proto protocol;
    if (container == DockerContainer::Cloak) {
        protocol = Proto::ShadowSocks;
    } else {
        protocol = ContainerProps::defaultProtocol(container);
    }

    auto nativeResult = generateNativeConfig(serverIndex, container, containerConfig, "", protocol, isApiConfig);
    if (nativeResult.errorCode != ErrorCode::NoError) {
        result.errorCode = nativeResult.errorCode;
        return result;
    }

    QStringList lines = QString(QJsonDocument(nativeResult.jsonNativeConfig).toJson()).replace("\r", "").split("\n");
    for (const QString &line : std::as_const(lines)) {
        result.config.append(line + "\n");
    }

    result.nativeConfigString = QString("%1:%2@%3:%4")
                                   .arg(nativeResult.jsonNativeConfig.value("method").toString(),
                                        nativeResult.jsonNativeConfig.value("password").toString(),
                                        nativeResult.jsonNativeConfig.value("server").toString(),
                                        nativeResult.jsonNativeConfig.value("server_port").toString());

    result.nativeConfigString = "ss://" + result.nativeConfigString.toUtf8().toBase64();

    result.qrCodes << generateSingleQrCode(result.nativeConfigString.toUtf8());
    return result;
}

ExportController::ExportResult ExportController::generateCloakConfig(int serverIndex, bool isApiConfig)
{
    ExportResult result;

    QJsonObject containerConfig = m_serversRepository->containerConfig(serverIndex, DockerContainer::Cloak);

    auto nativeResult = generateNativeConfig(serverIndex, DockerContainer::Cloak, containerConfig, "", Proto::Cloak, isApiConfig);
    if (nativeResult.errorCode != ErrorCode::NoError) {
        result.errorCode = nativeResult.errorCode;
        return result;
    }

    QJsonObject modifiedConfig = nativeResult.jsonNativeConfig;
    modifiedConfig.remove(config_key::transport_proto);
    modifiedConfig.insert("ProxyMethod", "shadowsocks");

    QStringList lines = QString(QJsonDocument(modifiedConfig).toJson()).replace("\r", "").split("\n");
    for (const QString &line : std::as_const(lines)) {
        result.config.append(line + "\n");
    }

    return result;
}

ExportController::ExportResult ExportController::generateXrayConfig(int serverIndex, const QString &clientName, bool isApiConfig)
{
    ExportResult result;

    QJsonObject containerConfig = m_serversRepository->containerConfig(serverIndex, DockerContainer::Xray);

    auto nativeResult = generateNativeConfig(serverIndex, DockerContainer::Xray, containerConfig, clientName, Proto::Xray, isApiConfig);
    if (nativeResult.errorCode != ErrorCode::NoError) {
        result.errorCode = nativeResult.errorCode;
        return result;
    }

    QStringList lines = QString(QJsonDocument(nativeResult.jsonNativeConfig).toJson()).replace("\r", "").split("\n");
    for (const QString &line : std::as_const(lines)) {
        result.config.append(line + "\n");
    }

    // Parse the Xray data to extract VLESS parameters and generate string
    QJsonObject xrayConfig = nativeResult.jsonNativeConfig;
    QJsonArray outbounds = xrayConfig.value("outbounds").toArray();

    if (outbounds.isEmpty()) {
        result.errorCode = ErrorCode::InternalError;
        return result;
    }

    QJsonObject outbound = outbounds[0].toObject();
    QJsonObject settings = outbound.value("settings").toObject();
    QJsonObject streamSettings = outbound.value("streamSettings").toObject();

    QJsonArray vnext = settings.value("vnext").toArray();
    if (vnext.isEmpty()) {
        result.errorCode = ErrorCode::InternalError;
        return result;
    }

    QJsonObject server = vnext[0].toObject();
    QJsonArray users = server.value("users").toArray();
    if (users.isEmpty()) {
        result.errorCode = ErrorCode::InternalError;
        return result;
    }

    QJsonObject user = users[0].toObject();

    amnezia::serialization::VlessServerObject vlessServer;
    vlessServer.address = server.value("address").toString();
    vlessServer.port = server.value("port").toInt();
    vlessServer.id = user.value("id").toString();
    vlessServer.flow = user.value("flow").toString("xtls-rprx-vision");
    vlessServer.encryption = user.value("encryption").toString("none");

    vlessServer.network = streamSettings.value("network").toString("tcp");
    vlessServer.security = streamSettings.value("security").toString("reality");

    if (vlessServer.security == "reality") {
        QJsonObject realitySettings = streamSettings.value("realitySettings").toObject();
        vlessServer.serverName = realitySettings.value("serverName").toString();
        vlessServer.publicKey = realitySettings.value("publicKey").toString();
        vlessServer.shortId = realitySettings.value("shortId").toString();
        vlessServer.fingerprint = realitySettings.value("fingerprint").toString("chrome");
        vlessServer.spiderX = realitySettings.value("spiderX").toString("");
    }

    result.nativeConfigString = amnezia::serialization::vless::Serialize(vlessServer, "AmneziaVPN");

    return result;
}

void ExportController::updateClientManagementModel(int serverIndex, int containerIndex)
{
    DockerContainer container = static_cast<DockerContainer>(containerIndex);
    ServerCredentials credentials = m_serversRepository->serverCredentials(serverIndex);
    emit updateClientsRequested(container, credentials);
}

void ExportController::revokeConfig(int row, int serverIndex, int containerIndex)
{
    DockerContainer container = static_cast<DockerContainer>(containerIndex);
    ServerCredentials credentials = m_serversRepository->serverCredentials(serverIndex);
    emit revokeClientRequested(row, container, credentials, serverIndex);
}

void ExportController::renameClient(int row, const QString &clientName, int serverIndex, int containerIndex)
{
    DockerContainer container = static_cast<DockerContainer>(containerIndex);
    ServerCredentials credentials = m_serversRepository->serverCredentials(serverIndex);
    emit renameClientRequested(row, clientName, container, credentials);
}

QString ExportController::generateVpnUrl(const QByteArray &compressedConfig)
{
    return QString("vpn://%1").arg(QString(compressedConfig.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals)));
}

QList<QString> ExportController::generateQrCodesFromConfig(const QByteArray &data)
{
    return qrCodeUtils::generateQrCodeImageSeries(data);
}

QString ExportController::generateSingleQrCode(const QByteArray &data)
{
    auto qr = qrCodeUtils::generateQrCode(data);
    return qrCodeUtils::svgToBase64(QString::fromStdString(toSvgString(qr, 1)));
}

QPair<QString, QString> ExportController::getDnsPair(int serverIndex, bool isAmneziaDnsEnabled) const
{
    QPair<QString, QString> dns;
    
    const QJsonObject &server = m_serversRepository->server(serverIndex);
    const auto containers = server.value(config_key::containers).toArray();
    
    bool isDnsContainerInstalled = false;
    for (const QJsonValue &container : containers) {
        if (ContainerProps::containerFromString(container.toObject().value(config_key::container).toString()) == DockerContainer::Dns) {
            isDnsContainerInstalled = true;
        }
    }
    
    dns.first = server.value(config_key::dns1).toString();
    dns.second = server.value(config_key::dns2).toString();
    
    if (dns.first.isEmpty() || !NetworkUtilities::checkIPv4Format(dns.first)) {
        if (isAmneziaDnsEnabled && isDnsContainerInstalled) {
            dns.first = protocols::dns::amneziaDnsIp;
        } else {
            dns.first = m_appSettingsRepository->primaryDns();
        }
    }
    
    if (dns.second.isEmpty() || !NetworkUtilities::checkIPv4Format(dns.second)) {
        dns.second = m_appSettingsRepository->secondaryDns();
    }
    
    return dns;
}
