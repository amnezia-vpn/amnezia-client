#include "macosSplitTunnelManager.h"

#import <Foundation/Foundation.h>
#import <NetworkExtension/NetworkExtension.h>
#import <SystemExtensions/SystemExtensions.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "logger.h"

#include <QDebug>

namespace {
Logger logger("MacOSSplitTunnelManager");

NSString *ProviderBundleId()
{
#ifdef CLIENT_MACOS_ST_BUNDLE_ID
    return @CLIENT_MACOS_ST_BUNDLE_ID;
#else
    return @"org.amnezia.AmneziaVPN.network-extension";
#endif
}

QString ErrorDump(NSError *error)
{
    if (error == nil) {
        return QStringLiteral("nil");
    }
    return QStringLiteral("%1 code=%2 desc=%3 userInfo=%4")
        .arg(QString::fromNSString(error.domain))
        .arg(error.code)
        .arg(QString::fromNSString(error.localizedDescription))
        .arg(QString::fromNSString(error.userInfo.description));
}

void LogApps(const char *where, const QVector<amnezia::InstalledAppInfo> &apps)
{
    logger.debug() << where << "count=" << apps.size();
    qDebug() << "MacOSSplitTunnel" << where << "count=" << apps.size();
    for (int i = 0; i < apps.size(); ++i) {
        const auto &app = apps.at(i);
        logger.debug() << where << i
                       << "name=" << app.appName
                       << "bundleId=" << app.packageName
                       << "path=" << app.appPath;
        qDebug() << "MacOSSplitTunnel" << where << i
                 << "name=" << app.appName
                 << "bundleId=" << app.packageName
                 << "path=" << app.appPath;
    }
}

void LogHostBundle()
{
    NSBundle *bundle = [NSBundle mainBundle];
    logger.debug() << "mainBundle=" << QString::fromNSString(bundle.bundlePath)
                   << "id=" << QString::fromNSString(bundle.bundleIdentifier);
    logger.debug() << "NSSystemExtensionUsageDescription="
                   << QString::fromNSString([bundle objectForInfoDictionaryKey:@"NSSystemExtensionUsageDescription"]);

    NSString *sysexPath = [bundle.bundlePath
        stringByAppendingPathComponent:@"Contents/Library/SystemExtensions/AmneziaVPNSplitTunnel.systemextension"];
    const bool exists = [[NSFileManager defaultManager] fileExistsAtPath:sysexPath];
    logger.debug() << "sysexPath=" << QString::fromNSString(sysexPath) << "exists=" << exists;
    if (exists) {
        NSBundle *sysex = [NSBundle bundleWithPath:sysexPath];
        logger.debug() << "sysexId=" << QString::fromNSString(sysex.bundleIdentifier)
                       << "version=" << QString::fromNSString(sysex.infoDictionary[@"CFBundleVersion"]);
    }
}

NSDictionary *OptionsDictionary(const QByteArray &json)
{
    if (json.isEmpty()) {
        logger.debug() << "OptionsDictionary empty json";
        return @{};
    }
    NSData *data = [NSData dataWithBytes:json.constData() length:static_cast<NSUInteger>(json.size())];
    NSError *error = nil;
    id object = [NSJSONSerialization JSONObjectWithData:data options:0 error:&error];
    if (error != nil) {
        logger.error() << "OptionsDictionary json parse failed:" << ErrorDump(error);
        return @{};
    }
    if (![object isKindOfClass:[NSDictionary class]]) {
        logger.error() << "OptionsDictionary json is not a dict";
        return @{};
    }
    return object;
}

const char *VpnStatusName(NEVPNStatus status)
{
    switch (status) {
    case NEVPNStatusInvalid: return "Invalid";
    case NEVPNStatusDisconnected: return "Disconnected";
    case NEVPNStatusConnecting: return "Connecting";
    case NEVPNStatusConnected: return "Connected";
    case NEVPNStatusReasserting: return "Reasserting";
    case NEVPNStatusDisconnecting: return "Disconnecting";
    default: return "Unknown";
    }
}
} // namespace

@interface AmneziaSTExtensionDelegate : NSObject <OSSystemExtensionRequestDelegate>
@property (nonatomic, copy) void (^finished)(BOOL ok, NSString *message, BOOL needsApproval);
@end

@implementation AmneziaSTExtensionDelegate

- (OSSystemExtensionReplacementAction)request:(OSSystemExtensionRequest *)request
                  actionForReplacingExtension:(OSSystemExtensionProperties *)existing
                                withExtension:(OSSystemExtensionProperties *)ext
{
    logger.debug() << "sysex replace existing=" << QString::fromNSString(existing.bundleIdentifier)
                   << QString::fromNSString(existing.bundleVersion) << "->"
                   << QString::fromNSString(ext.bundleIdentifier)
                   << QString::fromNSString(ext.bundleVersion);
    (void)request;
    return OSSystemExtensionReplacementActionReplace;
}

- (void)requestNeedsUserApproval:(OSSystemExtensionRequest *)request
{
    (void)request;
    logger.debug() << "sysex needs user approval";
    if (self.finished) {
        self.finished(NO, @"needsApproval", YES);
    }
}

- (void)request:(OSSystemExtensionRequest *)request didFailWithError:(NSError *)error
{
    (void)request;
                logger.error() << "sysex request failed:" << ErrorDump(error);
    qWarning() << "MacOSSplitTunnel sysex request failed:" << ErrorDump(error);
    if (self.finished) {
        self.finished(NO, error.localizedDescription, NO);
    }
}

- (void)request:(OSSystemExtensionRequest *)request didFinishWithResult:(OSSystemExtensionRequestResult)result
{
    (void)request;
    logger.debug() << "sysex request finished result=" << static_cast<int>(result);
    if (self.finished) {
        self.finished(YES, nil, NO);
    }
}

@end

static AmneziaSTExtensionDelegate *g_extensionDelegate = nil;

MacOSSplitTunnelManager *MacOSSplitTunnelManager::instance()
{
    static MacOSSplitTunnelManager *s_instance = new MacOSSplitTunnelManager(nullptr);
    return s_instance;
}

MacOSSplitTunnelManager::MacOSSplitTunnelManager(QObject *parent) : QObject(parent)
{
}

QByteArray MacOSSplitTunnelManager::optionsJson(const QVector<amnezia::InstalledAppInfo> &apps, const QString &vpnServer) const
{
    QJsonArray appsJson;
    for (const auto &app : apps) {
        QJsonObject entry;
        entry.insert(QStringLiteral("bundleId"), app.packageName);
        entry.insert(QStringLiteral("path"), app.appPath);
        appsJson.append(entry);
    }

    QJsonObject root;
    root.insert(QStringLiteral("mode"), QStringLiteral("except"));
    root.insert(QStringLiteral("apps"), appsJson);
    root.insert(QStringLiteral("vpnServer"), vpnServer);
    const QByteArray json = QJsonDocument(root).toJson(QJsonDocument::Compact);
    logger.debug() << "optionsJson=" << QString::fromUtf8(json);
    return json;
}

void MacOSSplitTunnelManager::activateExtension()
{
    if (@available(macOS 11.0, *)) {
        logger.debug() << "activateExtension requested already=" << m_activationRequested
                       << "provider=" << QString::fromNSString(ProviderBundleId());
        qDebug() << "MacOSSplitTunnel activateExtension already=" << m_activationRequested
                 << "provider=" << QString::fromNSString(ProviderBundleId());
        LogHostBundle();
        if (m_activationRequested) {
            logger.debug() << "activateExtension skipped: already requested";
            return;
        }
        m_activationRequested = true;

        g_extensionDelegate = [[AmneziaSTExtensionDelegate alloc] init];
        g_extensionDelegate.finished = ^(BOOL ok, NSString *message, BOOL needsApproval) {
            if (needsApproval) {
                logger.debug() << "System extension needs user approval";
                emit needsUserApproval();
                return;
            }
            if (!ok) {
                logger.error() << "System extension activation failed:" << QString::fromNSString(message);
                qWarning() << "MacOSSplitTunnel activation failed:" << QString::fromNSString(message);
                emit errorOccurred(QString::fromNSString(message));
                m_activationRequested = false;
            } else {
                logger.debug() << "System extension activated";
            }
        };

        OSSystemExtensionRequest *request =
            [OSSystemExtensionRequest activationRequestForExtension:ProviderBundleId() queue:dispatch_get_main_queue()];
        request.delegate = g_extensionDelegate;
        logger.debug() << "submit OSSystemExtensionRequest" << QString::fromNSString(ProviderBundleId());
        [[OSSystemExtensionManager sharedManager] submitRequest:request];
    }
}

void MacOSSplitTunnelManager::reconcile(bool vpnConnected, bool splitTunnelEnabled, amnezia::AppsRouteMode mode,
                                        const QVector<amnezia::InstalledAppInfo> &apps, const QString &vpnServer)
{
    const bool exceptMode = mode == amnezia::AppsRouteMode::VpnAllExceptApps;
    const bool shouldRun = vpnConnected && splitTunnelEnabled && exceptMode && !apps.isEmpty();
    logger.debug() << "reconcile vpnConnected=" << vpnConnected
                   << "splitEnabled=" << splitTunnelEnabled
                   << "mode=" << static_cast<int>(mode)
                   << "exceptMode=" << exceptMode
                   << "appsEmpty=" << apps.isEmpty()
                   << "vpnServer=" << vpnServer
                   << "shouldRun=" << shouldRun;
    qDebug() << "MacOSSplitTunnel reconcile vpnConnected=" << vpnConnected
             << "splitEnabled=" << splitTunnelEnabled
             << "mode=" << static_cast<int>(mode)
             << "exceptMode=" << exceptMode
             << "appsEmpty=" << apps.isEmpty()
             << "vpnServer=" << vpnServer
             << "shouldRun=" << shouldRun;
    LogApps("reconcile", apps);
    if (shouldRun) {
        activateExtension();
        startProxy(apps, vpnServer);
    } else {
        logger.debug() << "reconcile: stopProxy";
        stopProxy();
    }
}

void MacOSSplitTunnelManager::startProxy(const QVector<amnezia::InstalledAppInfo> &apps, const QString &vpnServer)
{
    if (@available(macOS 11.0, *)) {
        const QByteArray json = optionsJson(apps, vpnServer);
        NSDictionary *options = OptionsDictionary(json);
        NSString *bundleId = ProviderBundleId();
        logger.debug() << "startProxy provider=" << QString::fromNSString(bundleId)
                       << "optionsKeys=" << QString::fromNSString(options.allKeys.description);

        [NETransparentProxyManager loadAllFromPreferencesWithCompletionHandler:^(NSArray<NETransparentProxyManager *> *managers, NSError *loadError) {
            if (loadError != nil) {
                logger.error() << "loadAllFromPreferences failed:" << ErrorDump(loadError);
                emit errorOccurred(QString::fromNSString(loadError.localizedDescription));
                return;
            }

            logger.debug() << "loadAllFromPreferences count=" << (managers != nil ? (int)managers.count : 0);
            NETransparentProxyManager *manager = nil;
            int index = 0;
            for (NETransparentProxyManager *item in managers) {
                NETunnelProviderProtocol *proto = (NETunnelProviderProtocol *)item.protocolConfiguration;
                const QString protoId = [proto isKindOfClass:[NETunnelProviderProtocol class]]
                    ? QString::fromNSString(proto.providerBundleIdentifier)
                    : QStringLiteral("<not-tunnel-proto>");
                logger.debug() << "existing manager" << index++
                               << "enabled=" << item.enabled
                               << "status=" << VpnStatusName(item.connection.status)
                               << "provider=" << protoId
                               << "desc=" << QString::fromNSString(item.localizedDescription);
                if ([proto isKindOfClass:[NETunnelProviderProtocol class]]
                    && [proto.providerBundleIdentifier isEqualToString:bundleId]) {
                    manager = item;
                }
            }
            if (manager == nil) {
                logger.debug() << "creating new NETransparentProxyManager";
                manager = [[NETransparentProxyManager alloc] init];
            } else {
                logger.debug() << "reusing existing NETransparentProxyManager";
            }

            NETunnelProviderProtocol *protocol = [[NETunnelProviderProtocol alloc] init];
            protocol.providerBundleIdentifier = bundleId;
            protocol.serverAddress = @"AmneziaVPN Split Tunnel";
            protocol.providerConfiguration = options;

            manager.protocolConfiguration = protocol;
            manager.localizedDescription = @"AmneziaVPN Split Tunnel";
            manager.enabled = YES;

            [manager saveToPreferencesWithCompletionHandler:^(NSError *saveError) {
                if (saveError != nil) {
                    logger.error() << "saveToPreferences failed:" << ErrorDump(saveError);
                    qWarning() << "MacOSSplitTunnel saveToPreferences failed:" << ErrorDump(saveError);
                    emit errorOccurred(QString::fromNSString(saveError.localizedDescription));
                    return;
                }
                logger.debug() << "saveToPreferences ok";

                [manager loadFromPreferencesWithCompletionHandler:^(NSError *reloadError) {
                    if (reloadError != nil) {
                        logger.error() << "reload preferences failed:" << ErrorDump(reloadError);
                        emit errorOccurred(QString::fromNSString(reloadError.localizedDescription));
                        return;
                    }
                    logger.debug() << "reload preferences ok status=" << VpnStatusName(manager.connection.status);

                    NSError *startError = nil;
                    BOOL started = [manager.connection startVPNTunnelWithOptions:options andReturnError:&startError];
                    logger.debug() << "startVPNTunnel started=" << started
                                   << "status=" << VpnStatusName(manager.connection.status)
                                   << "error=" << ErrorDump(startError);
                    qDebug() << "MacOSSplitTunnel startVPNTunnel started=" << started
                             << "status=" << VpnStatusName(manager.connection.status)
                             << "error=" << ErrorDump(startError);
                    if (!started) {
                        logger.error() << "startVPNTunnel failed:" << ErrorDump(startError);
                        qWarning() << "MacOSSplitTunnel startVPNTunnel failed:" << ErrorDump(startError);
                        emit errorOccurred(startError != nil ? QString::fromNSString(startError.localizedDescription)
                                                             : QStringLiteral("startVPNTunnel returned NO"));
                    }
                }];
            }];
        }];
    }
}

void MacOSSplitTunnelManager::stopProxy()
{
    if (@available(macOS 11.0, *)) {
        NSString *bundleId = ProviderBundleId();
        logger.debug() << "stopProxy provider=" << QString::fromNSString(bundleId);
        [NETransparentProxyManager loadAllFromPreferencesWithCompletionHandler:^(NSArray<NETransparentProxyManager *> *managers, NSError *loadError) {
            if (loadError != nil) {
                logger.error() << "stopProxy loadAllFromPreferences failed:" << ErrorDump(loadError);
                return;
            }
            for (NETransparentProxyManager *item in managers) {
                NETunnelProviderProtocol *proto = (NETunnelProviderProtocol *)item.protocolConfiguration;
                if ([proto isKindOfClass:[NETunnelProviderProtocol class]]
                    && [proto.providerBundleIdentifier isEqualToString:bundleId]) {
                    logger.debug() << "stopVPNTunnel status=" << VpnStatusName(item.connection.status);
                    [item.connection stopVPNTunnel];
                }
            }
        }];
    }
}
