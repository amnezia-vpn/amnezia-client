#include "masterDnsVpnProtocolConfig.h"

#include <QJsonDocument>

#include "../../../core/protocols/protocolUtils.h"
#include "../../../core/utils/constants/configKeys.h"
#include "../../../core/utils/constants/protocolConstants.h"
#include "../../../core/utils/protocolEnum.h"

using namespace amnezia;
using namespace ProtocolUtils;

namespace amnezia
{

QJsonObject MasterDnsVpnServerConfig::toJson() const
{
    QJsonObject obj;

    if (!domains.isEmpty()) {
        obj[configKey::mdvDomains] = domains;
    }
    if (!port.isEmpty()) {
        obj[configKey::port] = port;
    }
    if (!bind.isEmpty()) {
        obj[configKey::mdvBind] = bind;
    }
    obj[configKey::mdvEncryptionMethod] = encryptionMethod;
    if (!encryptionKey.isEmpty()) {
        obj[configKey::mdvEncryptionKey] = encryptionKey;
    }
    if (!protocolType.isEmpty()) {
        obj[configKey::mdvProtocolType] = protocolType;
    }
    if (!dnsUpstreamServers.isEmpty()) {
        obj[configKey::mdvDnsUpstreamServers] = dnsUpstreamServers;
    }
    if (!forwardIp.isEmpty()) {
        obj[configKey::mdvForwardIp] = forwardIp;
    }
    if (forwardPort != 0) {
        obj[configKey::mdvForwardPort] = forwardPort;
    }
    if (useExternalSocks5) {
        obj[configKey::mdvUseExternalSocks5] = useExternalSocks5;
    }
    if (socks5Auth) {
        obj[configKey::mdvSocks5Auth] = socks5Auth;
    }
    if (!socks5User.isEmpty()) {
        obj[configKey::mdvSocks5User] = socks5User;
    }
    if (!socks5Pass.isEmpty()) {
        obj[configKey::mdvSocks5Pass] = socks5Pass;
    }
    if (!additionalConfig.isEmpty()) {
        obj[configKey::mdvAdditionalConfig] = additionalConfig;
    }
    if (isThirdPartyConfig) {

        obj[configKey::isThirdPartyConfig] = isThirdPartyConfig;
    }

    return obj;
}

MasterDnsVpnServerConfig MasterDnsVpnServerConfig::fromJson(const QJsonObject &json)
{
    MasterDnsVpnServerConfig config;

    config.domains = json.value(configKey::mdvDomains).toArray();
    config.port = json.value(configKey::port).toString();
    config.bind = json.value(configKey::mdvBind).toString();
    config.encryptionMethod = json.value(configKey::mdvEncryptionMethod)
                                      .toInt(protocols::masterDnsVpn::defaultEncryptionMethod);
    config.encryptionKey = json.value(configKey::mdvEncryptionKey).toString();
    config.protocolType = json.value(configKey::mdvProtocolType).toString();
    config.dnsUpstreamServers = json.value(configKey::mdvDnsUpstreamServers).toArray();
    config.forwardIp = json.value(configKey::mdvForwardIp).toString();
    config.forwardPort = json.value(configKey::mdvForwardPort).toInt(0);
    config.useExternalSocks5 = json.value(configKey::mdvUseExternalSocks5).toBool(false);
    config.socks5Auth = json.value(configKey::mdvSocks5Auth).toBool(false);
    config.socks5User = json.value(configKey::mdvSocks5User).toString();
    config.socks5Pass = json.value(configKey::mdvSocks5Pass).toString();
    config.additionalConfig = json.value(configKey::mdvAdditionalConfig).toObject();
    config.isThirdPartyConfig = json.value(configKey::isThirdPartyConfig).toBool(false);

    return config;
}

QJsonObject MasterDnsVpnClientConfig::toJson() const
{
    QJsonObject obj;

    if (!listenPort.isEmpty()) {
        obj[configKey::mdvListenPort] = listenPort;
    }
    if (!socks5User.isEmpty()) {
        obj[configKey::mdvSocks5User] = socks5User;
    }
    if (!socks5Pass.isEmpty()) {
        obj[configKey::mdvSocks5Pass] = socks5Pass;
    }
    if (!resolvers.isEmpty()) {
        obj[configKey::mdvResolvers] = resolvers;
    }
    if (balancingStrategy != 0) {
        obj[configKey::mdvBalancingStrategy] = balancingStrategy;
    }
    if (packetDuplication != 0) {
        obj[configKey::mdvPacketDuplication] = packetDuplication;
    }
    if (setupPacketDuplication != 0) {
        obj[configKey::mdvSetupPacketDuplication] = setupPacketDuplication;
    }
    if (uploadCompression != 0) {
        obj[configKey::mdvUploadCompression] = uploadCompression;
    }
    if (downloadCompression != 0) {
        obj[configKey::mdvDownloadCompression] = downloadCompression;
    }
    if (!additionalConfig.isEmpty()) {
        obj[configKey::mdvAdditionalConfig] = additionalConfig;
    }
    if (!id.isEmpty()) {
        obj[configKey::clientId] = id;
    }

    return obj;
}

MasterDnsVpnClientConfig MasterDnsVpnClientConfig::fromJson(const QJsonObject &json)
{
    MasterDnsVpnClientConfig config;

    config.listenPort = json.value(configKey::mdvListenPort).toString();
    config.socks5User = json.value(configKey::mdvSocks5User).toString();
    config.socks5Pass = json.value(configKey::mdvSocks5Pass).toString();
    config.resolvers = json.value(configKey::mdvResolvers).toArray();
    // Defaults match upstream sample config when an operator omits a field;
    // the engine clamps these to sane ranges before use.
    config.balancingStrategy = json.value(configKey::mdvBalancingStrategy).toInt(5);
    config.packetDuplication = json.value(configKey::mdvPacketDuplication).toInt(3);
    config.setupPacketDuplication = json.value(configKey::mdvSetupPacketDuplication).toInt(4);
    config.uploadCompression = json.value(configKey::mdvUploadCompression).toInt(0);
    config.downloadCompression = json.value(configKey::mdvDownloadCompression).toInt(0);
    config.additionalConfig = json.value(configKey::mdvAdditionalConfig).toObject();
    config.id = json.value(configKey::clientId).toString();

    return config;
}

QJsonObject MasterDnsVpnProtocolConfig::toJson() const
{
    QJsonObject obj = serverConfig.toJson();

    if (clientConfig.has_value()) {
        QJsonObject clientJson = clientConfig->toJson();
        obj[configKey::lastConfig] = QString::fromUtf8(QJsonDocument(clientJson).toJson(QJsonDocument::Compact));
    }

    return obj;
}

MasterDnsVpnProtocolConfig MasterDnsVpnProtocolConfig::fromJson(const QJsonObject &json)
{
    MasterDnsVpnProtocolConfig config;

    config.serverConfig = MasterDnsVpnServerConfig::fromJson(json);

    QString lastConfigStr = json.value(configKey::lastConfig).toString();
    if (!lastConfigStr.isEmpty()) {
        QJsonDocument doc = QJsonDocument::fromJson(lastConfigStr.toUtf8());
        if (doc.isObject()) {
            config.clientConfig = MasterDnsVpnClientConfig::fromJson(doc.object());
        }
    }

    return config;
}

bool MasterDnsVpnProtocolConfig::hasClientConfig() const
{
    return clientConfig.has_value();
}

void MasterDnsVpnProtocolConfig::setClientConfig(const MasterDnsVpnClientConfig &config)
{
    clientConfig = config;
}

void MasterDnsVpnProtocolConfig::clearClientConfig()
{
    clientConfig.reset();
}

} // namespace amnezia
