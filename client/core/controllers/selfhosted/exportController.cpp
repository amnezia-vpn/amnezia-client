#include "exportController.h"

#include <QJsonArray>
#include <QJsonDocument>

#include "core/configurators/configuratorBase.h"
#include "core/utils/selfhosted/sshSession.h"
#include "core/utils/qrCodeUtils.h"
#include "core/utils/serialization/serialization.h"
#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/models/selfhosted/selfHostedAdminServerConfig.h"
#include "core/models/containerConfig.h"
#include "core/models/protocolConfig.h"

using namespace amnezia;

ExportController::ExportController(SecureServersRepository* serversRepository,
                                   SecureAppSettingsRepository* appSettingsRepository,
                                   QObject *parent)
    : QObject(parent),
      m_serversRepository(serversRepository),
      m_appSettingsRepository(appSettingsRepository)
{
}

ExportController::ExportResult ExportController::generateFullAccessConfig(const QString &serverId)
{
    ExportResult result;

    auto adminConfig = m_serversRepository->selfHostedAdminConfig(serverId);
    if (!adminConfig.has_value()) {
        result.errorCode = ErrorCode::InternalError;
        return result;
    }
    for (auto it = adminConfig->containers.begin(); it != adminConfig->containers.end(); ++it) {
        it.value().protocolConfig.clearClientConfig();
    }

    QJsonObject serverJson = adminConfig->toJson();
    QByteArray compressedConfig = QJsonDocument(serverJson).toJson();
    compressedConfig = qCompress(compressedConfig, 8);
    result.config = generateVpnUrl(compressedConfig);
    result.qrCodes = generateQrCodesFromConfig(compressedConfig);

    return result;
}

ExportController::ExportResult ExportController::generateConnectionConfig(const QString &serverId, int containerIndex, const QString &clientName)
{
    ExportResult result;

    DockerContainer container = static_cast<DockerContainer>(containerIndex);
    auto adminConfig = m_serversRepository->selfHostedAdminConfig(serverId);
    if (!adminConfig.has_value()) {
        result.errorCode = ErrorCode::InternalError;
        return result;
    }
    const ServerCredentials credentials = adminConfig->credentials();
    if (!credentials.isValid()) {
        result.errorCode = ErrorCode::InternalError;
        return result;
    }
    ContainerConfig containerConfig = adminConfig->containerConfig(container);

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
        
        QString clientId = newProtocolConfig.clientId();
        if (!clientId.isEmpty()) {
            emit appendClientRequested(serverId, clientId, clientName, container);
        }
    }

    const QPair<QString, QString> dns = adminConfig->getDnsPair(m_appSettingsRepository->useAmneziaDns(),
                                                               m_appSettingsRepository->primaryDns(),
                                                               m_appSettingsRepository->secondaryDns());

    adminConfig->containers.clear();
    adminConfig->containers[container] = containerConfig;
    adminConfig->defaultContainer = container;
    adminConfig->userName.clear();
    adminConfig->password.clear();
    adminConfig->port = 0;

    adminConfig->dns1 = dns.first;
    adminConfig->dns2 = dns.second;

    QJsonObject serverJson = adminConfig->toJson();
    QByteArray compressedConfig = QJsonDocument(serverJson).toJson();
    compressedConfig = qCompress(compressedConfig, 8);
    result.config = generateVpnUrl(compressedConfig);
    result.qrCodes = generateQrCodesFromConfig(compressedConfig);

    return result;
}

ExportController::NativeConfigResult ExportController::generateNativeConfig(const QString &serverId, DockerContainer container,
                                                                             const ContainerConfig &containerConfig,
                                                                             const QString &clientName)
{
    NativeConfigResult result;

    if (ContainerUtils::containerService(container) == ServiceType::Other) {
        return result;
    }

    Proto protocol = ContainerUtils::defaultProtocol(container);

    auto adminConfig = m_serversRepository->selfHostedAdminConfig(serverId);
    if (!adminConfig.has_value()) {
        result.errorCode = ErrorCode::InternalError;
        return result;
    }
    const ServerCredentials credentials = adminConfig->credentials();
    if (!credentials.isValid()) {
        result.errorCode = ErrorCode::InternalError;
        return result;
    }
    const QPair<QString, QString> dns = adminConfig->getDnsPair(m_appSettingsRepository->useAmneziaDns(),
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

    ExportSettings exportSettings = { { dns.first, dns.second } };
    ProtocolConfig processedConfig = configurator->processConfigWithExportSettings(exportSettings, newProtocolConfig);

    if (protocol == Proto::OpenVpn || protocol == Proto::WireGuard || protocol == Proto::Awg) {
        result.jsonNativeConfig[configKey::config] = processedConfig.nativeConfig();
    } else {
        result.jsonNativeConfig = QJsonDocument::fromJson(processedConfig.nativeConfig().toUtf8()).object();
    }

    if (protocol == Proto::OpenVpn || protocol == Proto::WireGuard || protocol == Proto::Awg || protocol == Proto::Xray) {
        QString clientId = newProtocolConfig.clientId();
        if (!clientId.isEmpty()) {
            emit appendClientRequested(serverId, clientId, clientName, container);
        }
    }
    return result;
}

ExportController::ExportResult ExportController::generateOpenVpnConfig(const QString &serverId, const QString &clientName)
{
    ExportResult result;

    DockerContainer container = DockerContainer::OpenVpn;
    auto adminConfig = m_serversRepository->selfHostedAdminConfig(serverId);
    if (!adminConfig.has_value()) {
        result.errorCode = ErrorCode::InternalError;
        return result;
    }
    ContainerConfig containerConfig = adminConfig->containerConfig(container);

    auto nativeResult = generateNativeConfig(serverId, container, containerConfig, clientName);
    if (nativeResult.errorCode != ErrorCode::NoError) {
        result.errorCode = nativeResult.errorCode;
        return result;
    }

    QStringList lines = nativeResult.jsonNativeConfig.value(configKey::config).toString().replace("\r", "").split("\n");
    for (const QString &line : std::as_const(lines)) {
        result.config.append(line + "\n");
    }

    result.qrCodes = generateQrCodesFromConfig(result.config.toUtf8());
    return result;
}

ExportController::ExportResult ExportController::generateWireGuardConfig(const QString &serverId, const QString &clientName)
{
    ExportResult result;

    auto adminConfig = m_serversRepository->selfHostedAdminConfig(serverId);
    if (!adminConfig.has_value()) {
        result.errorCode = ErrorCode::InternalError;
        return result;
    }
    ContainerConfig containerConfig = adminConfig->containerConfig(DockerContainer::WireGuard);

    auto nativeResult = generateNativeConfig(serverId, DockerContainer::WireGuard, containerConfig, clientName);
    if (nativeResult.errorCode != ErrorCode::NoError) {
        result.errorCode = nativeResult.errorCode;
        return result;
    }

    QStringList lines = nativeResult.jsonNativeConfig.value(configKey::config).toString().replace("\r", "").split("\n");
    for (const QString &line : std::as_const(lines)) {
        result.config.append(line + "\n");
    }

    result.qrCodes << generateSingleQrCode(result.config.toUtf8());
    return result;
}

ExportController::ExportResult ExportController::generateAwgConfig(const QString &serverId, int containerIndex, const QString &clientName)
{
    ExportResult result;

    DockerContainer container = static_cast<DockerContainer>(containerIndex);
    if (container != DockerContainer::Awg && container != DockerContainer::Awg2) {
        result.errorCode = ErrorCode::InternalError;
        return result;
    }
    auto adminConfig = m_serversRepository->selfHostedAdminConfig(serverId);
    if (!adminConfig.has_value()) {
        result.errorCode = ErrorCode::InternalError;
        return result;
    }
    ContainerConfig containerConfig = adminConfig->containerConfig(container);

    auto nativeResult = generateNativeConfig(serverId, container, containerConfig, clientName);
    if (nativeResult.errorCode != ErrorCode::NoError) {
        result.errorCode = nativeResult.errorCode;
        return result;
    }

    QStringList lines = nativeResult.jsonNativeConfig.value(configKey::config).toString().replace("\r", "").split("\n");
    for (const QString &line : std::as_const(lines)) {
        result.config.append(line + "\n");
    }

    result.qrCodes << generateSingleQrCode(result.config.toUtf8());
    return result;
}


ExportController::ExportResult ExportController::generateXrayConfig(const QString &serverId, const QString &clientName)
{
    ExportResult result;

    auto adminConfig = m_serversRepository->selfHostedAdminConfig(serverId);
    if (!adminConfig.has_value()) {
        result.errorCode = ErrorCode::InternalError;
        return result;
    }
    ContainerConfig containerConfig = adminConfig->containerConfig(DockerContainer::Xray);

    auto nativeResult = generateNativeConfig(serverId, DockerContainer::Xray, containerConfig, clientName);
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
    QJsonArray outbounds = xrayConfig.value(amnezia::protocols::xray::outbounds).toArray();

    if (outbounds.isEmpty()) {
        result.errorCode = ErrorCode::InternalError;
        return result;
    }

    QJsonObject outbound = outbounds[0].toObject();
    QJsonObject settings = outbound.value(amnezia::protocols::xray::settings).toObject();
    QJsonObject streamSettings = outbound.value(amnezia::protocols::xray::streamSettings).toObject();

    QJsonArray vnext = settings.value(amnezia::protocols::xray::vnext).toArray();
    if (vnext.isEmpty()) {
        result.errorCode = ErrorCode::InternalError;
        return result;
    }

    QJsonObject server = vnext[0].toObject();
    QJsonArray users = server.value(amnezia::protocols::xray::users).toArray();
    if (users.isEmpty()) {
        result.errorCode = ErrorCode::InternalError;
        return result;
    }

    QJsonObject user = users[0].toObject();

    amnezia::serialization::VlessServerObject vlessServer;
    vlessServer.address = server.value(amnezia::protocols::xray::address).toString();
    vlessServer.port = server.value(amnezia::protocols::xray::port).toInt();
    vlessServer.id = user.value(amnezia::protocols::xray::id).toString();
    vlessServer.flow = user.value(amnezia::protocols::xray::flow).toString("xtls-rprx-vision");
    vlessServer.encryption = user.value(amnezia::protocols::xray::encryption).toString("none");

    vlessServer.network = streamSettings.value(amnezia::protocols::xray::network).toString("tcp");
    vlessServer.security = streamSettings.value(amnezia::protocols::xray::security).toString("reality");

    if (vlessServer.security == "reality") {
        QJsonObject realitySettings = streamSettings.value(amnezia::protocols::xray::realitySettings).toObject();
        vlessServer.serverName = realitySettings.value(amnezia::protocols::xray::serverName).toString();
        vlessServer.publicKey = realitySettings.value(amnezia::protocols::xray::publicKey).toString();
        vlessServer.shortId = realitySettings.value(amnezia::protocols::xray::shortId).toString();
        vlessServer.fingerprint = realitySettings.value(amnezia::protocols::xray::fingerprint).toString("chrome");
        vlessServer.spiderX = realitySettings.value(amnezia::protocols::xray::spiderX).toString("");
    } else if (vlessServer.security == "tls") {
        QJsonObject tlsSettings = streamSettings.value("tlsSettings").toObject();
        vlessServer.serverName = tlsSettings.value(amnezia::protocols::xray::serverName).toString();
        vlessServer.fingerprint = tlsSettings.value(amnezia::protocols::xray::fingerprint).toString();
        // alpn: serialize array back to comma-separated for VLESS URI
        QJsonArray alpnArr = tlsSettings.value("alpn").toArray();
        QStringList alpnList;
        for (const QJsonValue &v : alpnArr) {
            alpnList << v.toString();
        }
        // alpn goes into vless URI query param — handled by Serialize via serverName/alpn fields
        // VlessServerObject doesn't have alpn field, so we embed in serverName if needed
    }

    result.nativeConfigString = amnezia::serialization::vless::Serialize(vlessServer, "AmneziaVPN");

    return result;
}

void ExportController::updateClientManagementModel(const QString &serverId, int containerIndex)
{
    DockerContainer container = static_cast<DockerContainer>(containerIndex);
    emit updateClientsRequested(serverId, container);
}

void ExportController::revokeConfig(int row, const QString &serverId, int containerIndex)
{
    DockerContainer container = static_cast<DockerContainer>(containerIndex);
    emit revokeClientRequested(serverId, row, container);
}

void ExportController::renameClient(int row, const QString &clientName, const QString &serverId, int containerIndex)
{
    DockerContainer container = static_cast<DockerContainer>(containerIndex);
    emit renameClientRequested(serverId, row, clientName, container);
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
