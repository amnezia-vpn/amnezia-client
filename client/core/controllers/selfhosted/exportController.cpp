#include "exportController.h"

#include <QJsonArray>
#include <QJsonDocument>

#include "core/configurators/configuratorBase.h"
#include "core/utils/selfhosted/sshSession.h"
#include "core/utils/networkUtilities.h"
#include "core/utils/qrCodeUtils.h"
#include "core/utils/serialization/serialization.h"
#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/models/serverConfig.h"
#include "core/models/containerConfig.h"
#include "core/models/protocolConfig.h"

ExportController::ExportController(SecureServersRepository* serversRepository,
                                   SecureAppSettingsRepository* appSettingsRepository,
                                   QObject *parent)
    : QObject(parent),
      m_serversRepository(serversRepository),
      m_appSettingsRepository(appSettingsRepository)
{
}

ExportController::ExportResult ExportController::generateFullAccessConfig(int serverIndex)
{
    ExportResult result;

    ServerConfig serverConfig = m_serversRepository->server(serverIndex);
    ServerConfigUtils::visit(serverConfig, [](auto& arg) {
        for (auto it = arg.containers.begin(); it != arg.containers.end(); ++it) {
            ProtocolConfigUtils::clearClientConfig(it.value().protocolConfig);
        }
    });

    QJsonObject serverJson = ServerConfigUtils::toJson(serverConfig);
    QByteArray compressedConfig = QJsonDocument(serverJson).toJson();
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
    ContainerConfig containerConfig = m_serversRepository->containerConfig(serverIndex, container);

    if (ContainerUtils::containerService(container) != ServiceType::Other) {
        SshSession sshSession;
        Proto protocol = ContainerUtils::defaultProtocol(container);

        DnsSettings dnsSettings = {
            m_appSettingsRepository->primaryDns(),
            m_appSettingsRepository->secondaryDns()
        };

        auto configurator = ConfiguratorBase::create(protocol, &sshSession);
        ProtocolConfig newProtocolConfig = configurator->createConfig(credentials, container, containerConfig, dnsSettings, result.errorCode);
        if (result.errorCode != ErrorCode::NoError) {
            return result;
        }

        containerConfig.protocolConfig = newProtocolConfig;
        
        QString clientId = ProtocolConfigUtils::clientId(newProtocolConfig);
        if (!clientId.isEmpty()) {
            emit appendClientRequested(serverIndex, clientId, clientName, container);
        }
    }

    ServerConfig serverConfig = m_serversRepository->server(serverIndex);
    ServerConfigUtils::visit(serverConfig, [container, containerConfig](auto& arg) {
        arg.containers.clear();
        arg.containers[container] = containerConfig;
        arg.defaultContainer = container;
    });

    if (ServerConfigUtils::isSelfHosted(serverConfig)) {
        SelfHostedServerConfig& selfHosted = ServerConfigUtils::asSelfHosted(serverConfig);
        selfHosted.userName.reset();
        selfHosted.password.reset();
        selfHosted.port.reset();
    }

    auto dns = ServerConfigUtils::getDnsPair(serverConfig,
                                              m_appSettingsRepository->useAmneziaDns(),
                                              m_appSettingsRepository->primaryDns(),
                                              m_appSettingsRepository->secondaryDns());
    ServerConfigUtils::visit(serverConfig, [&dns](auto& arg) {
        arg.dns1 = dns.first;
        arg.dns2 = dns.second;
    });

    QJsonObject serverJson = ServerConfigUtils::toJson(serverConfig);
    QByteArray compressedConfig = QJsonDocument(serverJson).toJson();
    compressedConfig = qCompress(compressedConfig, 8);
    result.config = generateVpnUrl(compressedConfig);
    result.qrCodes = generateQrCodesFromConfig(compressedConfig);

    return result;
}

ExportController::NativeConfigResult ExportController::generateNativeConfig(int serverIndex, DockerContainer container,
                                                                             const ContainerConfig &containerConfig,
                                                                             const QString &clientName, Proto protocol,
                                                                             bool isApiConfig)
{
    NativeConfigResult result;

    if (ContainerUtils::containerService(container) == ServiceType::Other) {
        return result;
    }

    ServerCredentials credentials = m_serversRepository->serverCredentials(serverIndex);
    ServerConfig serverConfig = m_serversRepository->server(serverIndex);
    auto dns = ServerConfigUtils::getDnsPair(serverConfig,
                                              m_appSettingsRepository->useAmneziaDns(),
                                              m_appSettingsRepository->primaryDns(),
                                              m_appSettingsRepository->secondaryDns());

    ContainerConfig modifiedContainerConfig = containerConfig;
    modifiedContainerConfig.container = container;

    DnsSettings dnsSettings = {
        m_appSettingsRepository->primaryDns(),
        m_appSettingsRepository->secondaryDns()
    };

    SshSession sshSession;
    auto configurator = ConfiguratorBase::create(protocol, &sshSession);

    ProtocolConfig newProtocolConfig = configurator->createConfig(credentials, container, modifiedContainerConfig, dnsSettings, result.errorCode);
    if (result.errorCode != ErrorCode::NoError) {
        return result;
    }
    
    QJsonObject protocolConfigJson = ProtocolConfigUtils::toJson(newProtocolConfig, protocol);
    QString protocolConfigString = QString::fromUtf8(QJsonDocument(protocolConfigJson).toJson(QJsonDocument::Compact));
    protocolConfigString = configurator->processConfigWithExportSettings(dns, isApiConfig, protocolConfigString);

    result.jsonNativeConfig = QJsonDocument::fromJson(protocolConfigString.toUtf8()).object();

    if (protocol == Proto::OpenVpn || protocol == Proto::WireGuard || protocol == Proto::Awg || protocol == Proto::Xray) {
        QString clientId = ProtocolConfigUtils::clientId(newProtocolConfig);
        if (!clientId.isEmpty()) {
            emit appendClientRequested(serverIndex, clientId, clientName, container);
        }
    }
    return result;
}

ExportController::ExportResult ExportController::generateOpenVpnConfig(int serverIndex, int containerIndex,
                                                                       const QString &clientName, bool isApiConfig)
{
    ExportResult result;

    DockerContainer container = static_cast<DockerContainer>(containerIndex);
    ContainerConfig containerConfig = m_serversRepository->containerConfig(serverIndex, container);

    Proto protocol = ContainerUtils::defaultProtocol(container);

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

    ContainerConfig containerConfig = m_serversRepository->containerConfig(serverIndex, DockerContainer::WireGuard);

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
    ContainerConfig containerConfig = m_serversRepository->containerConfig(serverIndex, container);

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


ExportController::ExportResult ExportController::generateXrayConfig(int serverIndex, const QString &clientName, bool isApiConfig)
{
    ExportResult result;

    ContainerConfig containerConfig = m_serversRepository->containerConfig(serverIndex, DockerContainer::Xray);

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
    emit updateClientsRequested(serverIndex, container);
}

void ExportController::revokeConfig(int row, int serverIndex, int containerIndex)
{
    DockerContainer container = static_cast<DockerContainer>(containerIndex);
    emit revokeClientRequested(serverIndex, row, container);
}

void ExportController::renameClient(int row, const QString &clientName, int serverIndex, int containerIndex)
{
    DockerContainer container = static_cast<DockerContainer>(containerIndex);
    emit renameClientRequested(serverIndex, row, clientName, container);
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
