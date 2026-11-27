#include "tunnelSettings.h"

#include <QJsonArray>
#include <QJsonDocument>

#include "core/protocols/protocolUtils.h"
#include "core/repositories/secureAppSettingsRepository.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/networkUtilities.h"

namespace amnezia
{
    namespace
    {
        XtunProto xtunProto(Proto proto)
        {
            switch (proto) {
            case Proto::Awg:
            case Proto::WireGuard: return XtunProto::Awg;
            case Proto::Xray:
            case Proto::SSXray: return XtunProto::Xray;
            default: return XtunProto::None;
            }
        }

        QString xtunProtoName(XtunProto proto)
        {
            switch (proto) {
            case XtunProto::Awg: return QStringLiteral("awg");
            case XtunProto::Xray: return QStringLiteral("xray");
            case XtunProto::None: return QString();
            }
            return QString();
        }

        QString xtunConfig(XtunProto proto, const QString &config)
        {
            if (proto != XtunProto::Xray) {
                return config;
            }

            QJsonObject root = QJsonDocument::fromJson(config.toUtf8()).object();
            QJsonArray inbounds = root.value(protocols::xray::inbounds).toArray();
            for (qsizetype i = 0; i < inbounds.size(); ++i) {
                QJsonObject inbound = inbounds.at(i).toObject();
                if (inbound.value(protocols::xray::protocol).toString() == protocols::xray::protocolSocks) {
                    inbound.insert(protocols::xray::tag, QStringLiteral("xtun"));
                    inbounds.replace(i, inbound);
                    break;
                }
            }
            root.insert(protocols::xray::inbounds, inbounds);
            return QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Compact));
        }

        QStringList dnsServers(const QJsonObject &vpnConfiguration)
        {
            const QString primaryDns = vpnConfiguration.value(configKey::dns1).toString();
            const QString secondaryDns = vpnConfiguration.value(configKey::dns2).toString();
            QStringList servers { primaryDns };
            if (!primaryDns.contains(protocols::dns::amneziaDnsIp) && !secondaryDns.isEmpty()) {
                servers.append(secondaryDns);
            }
            return servers;
        }

        QStringList splitTunnelingSites(const SecureAppSettingsRepository &appSettingsRepository, RouteMode routeMode,
                                        const QJsonObject &vpnConfiguration)
        {
            QStringList sites;
            const QVariantMap vpnSites = appSettingsRepository.vpnSites(routeMode);
            for (auto it = vpnSites.constBegin(); it != vpnSites.constEnd(); ++it) {
                if (NetworkUtilities::checkIpSubnetFormat(it.key())) {
                    sites.append(it.key());
                    continue;
                }
                for (const QString &ip : SecureAppSettingsRepository::siteIpList(it.value())) {
                    if (NetworkUtilities::checkIpSubnetFormat(ip)) {
                        sites.append(ip);
                    }
                }
            }
            sites.removeDuplicates();

            if (!sites.isEmpty() && routeMode == RouteMode::VpnOnlyForwardSites) {
                sites.append(vpnConfiguration.value(configKey::dns1).toString());
                sites.append(vpnConfiguration.value(configKey::dns2).toString());
            }
            return sites;
        }

        QStringList splitTunnelingApps(const SecureAppSettingsRepository &appSettingsRepository,
                                       AppsRouteMode appsRouteMode)
        {
            QStringList apps;
            for (const auto &app : appSettingsRepository.vpnApps(appsRouteMode)) {
                apps.append(app.appPath.isEmpty() ? app.packageName : app.appPath);
            }
            return apps;
        }
    }

    QJsonObject TunnelSettings::toJson() const
    {
        return QJsonObject {
            { "proto", xtunProtoName(proto) },
            { configKey::config, config },
            { "dns", QJsonArray::fromStringList(dnsServers) },
            { "killSwitchMode", static_cast<int>(killSwitchMode) },
            { configKey::allowedDnsServers, QJsonArray::fromStringList(allowedDnsServers) },
            { "includeRoutes", QJsonArray::fromStringList(includeRoutes) },
            { "excludeRoutes", QJsonArray::fromStringList(excludeRoutes) },
            { configKey::appSplitTunnelType, static_cast<int>(appsRouteMode) },
            { configKey::splitTunnelApps, QJsonArray::fromStringList(apps) },
            { configKey::mtu, mtu },
        };
    }

    TunnelSettings tunnelSettings(const SecureAppSettingsRepository &appSettingsRepository, DockerContainer container,
                                  const QJsonObject &vpnConfiguration)
    {
        const Proto proto = ContainerUtils::defaultProtocol(container);
        const QJsonObject protocolConfig = vpnConfiguration.value(ProtocolUtils::key_proto_config_data(proto)).toObject();

        TunnelSettings settings;
        settings.proto = xtunProto(proto);
        settings.config = xtunConfig(settings.proto, protocolConfig.value(configKey::config).toString());
        settings.dnsServers = dnsServers(vpnConfiguration);
        settings.killSwitchMode = killSwitchMode(appSettingsRepository, appSettingsRepository.isKillSwitchEnabled());
        settings.allowedDnsServers = appSettingsRepository.getAllowedDnsServers();
        settings.mtu = protocolConfig.value(configKey::mtu).toString().toInt();

        if (appSettingsRepository.isSitesSplitTunnelingEnabled()) {
            const RouteMode routeMode = appSettingsRepository.routeMode();
            const QStringList sites = splitTunnelingSites(appSettingsRepository, routeMode, vpnConfiguration);
            if (routeMode == RouteMode::VpnOnlyForwardSites) {
                settings.includeRoutes = sites;
            } else if (routeMode == RouteMode::VpnAllExceptSites) {
                settings.excludeRoutes = sites;
            }
        }

        if (appSettingsRepository.isAppsSplitTunnelingEnabled()) {
            settings.apps = splitTunnelingApps(appSettingsRepository, appSettingsRepository.appsRouteMode());
            if (!settings.apps.isEmpty()) {
                settings.appsRouteMode = appSettingsRepository.appsRouteMode();
            }
        }

        return settings;
    }

    KillSwitchMode killSwitchMode(const SecureAppSettingsRepository &appSettingsRepository, bool killSwitchEnabled)
    {
        if (!killSwitchEnabled) {
            return KillSwitchMode::Disabled;
        }
        return appSettingsRepository.isStrictKillSwitchEnabled() ? KillSwitchMode::Hard : KillSwitchMode::Soft;
    }
}
