#include "configurator_base.h"

ConfiguratorBase::ConfiguratorBase(std::shared_ptr<Settings> settings, const QSharedPointer<ServerController> &serverController,
                                   QObject *parent)
    : QObject { parent }, m_settings(settings), m_serverController(serverController)
{
}

QSharedPointer<ProtocolConfig> ConfiguratorBase::processConfigWithLocalSettings(const QPair<QString, QString> &dns, const bool isApiConfig,
                                                                                QSharedPointer<ProtocolConfig> protocolConfig)
{
    if (protocolConfig) {
        auto nativeConfig = ProtocolConfig::getNativeConfig(protocolConfig);
        if (!nativeConfig.isEmpty()) {
            processConfigWithDnsSettings(dns, nativeConfig);
            updateNativeConfig(protocolConfig, nativeConfig);
        }
    }
    return protocolConfig;
}

QSharedPointer<ProtocolConfig> ConfiguratorBase::processConfigWithExportSettings(const QPair<QString, QString> &dns,
                                                                                 QSharedPointer<ProtocolConfig> protocolConfig)
{
    if (protocolConfig) {
        auto nativeConfig = ProtocolConfig::getNativeConfig(protocolConfig);
        if (!nativeConfig.isEmpty()) {
            processConfigWithDnsSettings(dns, nativeConfig);
            updateNativeConfig(protocolConfig, nativeConfig);
        }
    }
    return protocolConfig;
}

void ConfiguratorBase::processConfigWithDnsSettings(const QPair<QString, QString> &dns, QString &protocolConfigString)
{
    protocolConfigString.replace("$PRIMARY_DNS", dns.first);
    protocolConfigString.replace("$SECONDARY_DNS", dns.second);
}

void ConfiguratorBase::updateNativeConfig(QSharedPointer<ProtocolConfig> protocolConfig, const QString &nativeConfig)
{
    if (!protocolConfig) {
        return;
    }

    auto protocolConfigVariant = ProtocolConfig::getProtocolConfigVariant(protocolConfig);
    std::visit(
            [&nativeConfig](const auto &configPtr) {
                if constexpr (requires { configPtr->clientProtocolConfig.nativeConfig; }) {
                    configPtr->clientProtocolConfig.nativeConfig = nativeConfig;
                }
            },
            protocolConfigVariant);
}
