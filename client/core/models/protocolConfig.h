#ifndef PROTOCOLCONFIG_H
#define PROTOCOLCONFIG_H

#include <QJsonObject>
#include <variant>
#include <type_traits>

#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"

#include "core/models/protocols/awgProtocolConfig.h"
#include "core/models/protocols/wireGuardProtocolConfig.h"
#include "core/models/protocols/openVpnProtocolConfig.h"
#include "core/models/protocols/xrayProtocolConfig.h"
#include "core/models/protocols/sftpProtocolConfig.h"
#include "core/models/protocols/socks5ProxyProtocolConfig.h"
#include "core/models/protocols/ikev2ProtocolConfig.h"
#include "core/models/protocols/torProtocolConfig.h"
#include "core/models/protocols/dnsProtocolConfig.h"
#include "core/models/protocols/mtProxyProtocolConfig.h"

namespace amnezia
{

using Proto = ProtocolEnumNS::Proto;

struct ProtocolConfig {
    using Variant = std::variant<
        AwgProtocolConfig,
        WireGuardProtocolConfig,
        OpenVpnProtocolConfig,
        XrayProtocolConfig,
        SftpProtocolConfig,
        Socks5ProxyProtocolConfig,
        MtProxyProtocolConfig,
        Ikev2ProtocolConfig,
        TorProtocolConfig,
        DnsProtocolConfig
    >;
    
    Variant data;
    
    ProtocolConfig() = default;
    ProtocolConfig(const Variant& v) : data(v) {}
    ProtocolConfig(Variant&& v) : data(std::move(v)) {}
    
    template<typename T, typename = std::enable_if_t<!std::is_same<std::remove_cv_t<std::remove_reference_t<T>>, ProtocolConfig>::value>>
    ProtocolConfig(const T& v) : data(v) {}
    
    template<typename T, typename = std::enable_if_t<!std::is_same<std::remove_cv_t<std::remove_reference_t<T>>, ProtocolConfig>::value>>
    ProtocolConfig(T&& v) : data(std::forward<T>(v)) {}
    
    Proto type() const;
    
    QString port() const;
    QString transportProto() const;
    
    bool hasClientConfig() const;
    QString clientId() const;
    QJsonObject getClientConfigJson() const;
    void setClientConfigJson(const QJsonObject& json);
    void clearClientConfig();
    
    QString nativeConfig() const;
    void setNativeConfig(const QString &config);

    bool isThirdPartyConfig() const;
    
    QJsonObject toJson() const;
    static ProtocolConfig fromJson(const QJsonObject& json, Proto type);
    
    template<typename T>
    T* as() {
        return std::get_if<T>(&data);
    }
    
    template<typename T>
    const T* as() const {
        return std::get_if<T>(&data);
    }
};

} // namespace amnezia

#endif // PROTOCOLCONFIG_H
