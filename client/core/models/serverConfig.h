#ifndef SERVERCONFIG_H
#define SERVERCONFIG_H

#include <variant>
#include <QJsonObject>
#include <QMap>

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/models/selfhosted/selfHostedServerConfig.h"
#include "core/models/selfhosted/nativeServerConfig.h"
#include "core/models/api/apiV1ServerConfig.h"
#include "core/models/api/apiV2ServerConfig.h"
#include "core/models/containerConfig.h"

namespace amnezia
{

using namespace ContainerEnumNS;

struct ServerConfig {
    using Variant = std::variant<
        SelfHostedServerConfig,
        NativeServerConfig,
        ApiV1ServerConfig,
        ApiV2ServerConfig
    >;
    
    Variant data;
    
    ServerConfig() = default;
    ServerConfig(const Variant& v) : data(v) {}
    ServerConfig(Variant&& v) : data(std::move(v)) {}
    
    template<typename T, typename = std::enable_if_t<!std::is_same<std::remove_cv_t<std::remove_reference_t<T>>, ServerConfig>::value>>
    ServerConfig(const T& v) : data(v) {}
    
    template<typename T, typename = std::enable_if_t<!std::is_same<std::remove_cv_t<std::remove_reference_t<T>>, ServerConfig>::value>>
    ServerConfig(T&& v) : data(std::forward<T>(v)) {}
    
    QString description() const;
    QString hostName() const;
    QString displayName() const;
    QMap<DockerContainer, ContainerConfig> containers() const;
    DockerContainer defaultContainer() const;
    QString dns1() const;
    QString dns2() const;
    bool hasContainers() const;
    ContainerConfig containerConfig(DockerContainer container) const;
    
    int crc() const;
    int configVersion() const;
    
    bool isSelfHosted() const;
    bool isNative() const;
    bool isApiV1() const;
    bool isApiV2() const;
    bool isApiConfig() const;
    
    template<typename T>
    T* as() {
        return std::get_if<T>(&data);
    }
    
    template<typename T>
    const T* as() const {
        return std::get_if<T>(&data);
    }
    
    QJsonObject toJson() const;
    static ServerConfig fromJson(const QJsonObject& json);
    
    template<typename Visitor>
    auto visit(Visitor&& visitor) {
        return std::visit(std::forward<Visitor>(visitor), data);
    }
    
    template<typename Visitor>
    auto visit(Visitor&& visitor) const {
        return std::visit(std::forward<Visitor>(visitor), data);
    }
    
    QPair<QString, QString> getDnsPair(bool isAmneziaDnsEnabled,
                                       const QString &primaryDns,
                                       const QString &secondaryDns) const;
};

} // namespace amnezia

#endif // SERVERCONFIG_H

