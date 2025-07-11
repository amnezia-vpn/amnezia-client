#ifndef APIDEFS_H
#define APIDEFS_H

#include <QString>

namespace apiDefs
{
    enum ConfigType {
        AmneziaFreeV2 = 0,
        AmneziaFreeV3,
        AmneziaPremiumV1,
        AmneziaPremiumV2,
        SelfHosted,
        ExternalPremium
    };

    enum ConfigSource {
        Telegram = 1,
        AmneziaGateway
    };

    namespace key
    {
        constexpr QLatin1String configVersion("config_version");
        constexpr QLatin1String apiEndpoint("api_endpoint");
        constexpr QLatin1String apiKey("api_key");
        constexpr QLatin1String description("description");
        constexpr QLatin1String name("name");
        constexpr QLatin1String protocol("protocol");

        constexpr QLatin1String apiConfig("api_config");
        constexpr QLatin1String stackType("stack_type");
        constexpr QLatin1String serviceType("service_type");
        constexpr QLatin1String cliVersion("cli_version");
        constexpr QLatin1String supportedProtocols("supported_protocols");

        constexpr QLatin1String vpnKey("vpn_key");
        constexpr QLatin1String config("config");
        constexpr QLatin1String configs("configs");

        constexpr QLatin1String installationUuid("installation_uuid");
        constexpr QLatin1String workerLastUpdated("worker_last_updated");
        constexpr QLatin1String lastDownloaded("last_downloaded");
        constexpr QLatin1String sourceType("source_type");

        constexpr QLatin1String serverCountryCode("server_country_code");
        constexpr QLatin1String serverCountryName("server_country_name");

        constexpr QLatin1String osVersion("os_version");

        constexpr QLatin1String availableCountries("available_countries");
        constexpr QLatin1String activeDeviceCount("active_device_count");
        constexpr QLatin1String maxDeviceCount("max_device_count");
        constexpr QLatin1String subscriptionEndDate("subscription_end_date");
        constexpr QLatin1String issuedConfigs("issued_configs");

        constexpr QLatin1String supportInfo("support_info");
        constexpr QLatin1String email("email");
        constexpr QLatin1String billingEmail("billing_email");
        constexpr QLatin1String website("website");
        constexpr QLatin1String websiteName("website_name");
        constexpr QLatin1String telegram("telegram");

        constexpr QLatin1String id("id");
        constexpr QLatin1String orderId("order_id");
        constexpr QLatin1String migrationCode("migration_code");
    }

    const int requestTimeoutMsecs = 12 * 1000; // 12 secs
}

#endif // APIDEFS_H
