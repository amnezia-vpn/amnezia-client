#include "macosSplitTunnelManager.h"

#import <Foundation/Foundation.h>
#import <NetworkExtension/NetworkExtension.h>
#import <SystemExtensions/SystemExtensions.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>
#include <QThread>

#include "logger.h"

namespace {
Logger logger("MacOSSplitTunnelManager");

NSString *ProviderBundleId()
{
#ifdef CLIENT_MACOS_ST_BUNDLE_ID
    return @CLIENT_MACOS_ST_BUNDLE_ID;
#else
    return @"org.amnezia.AmneziaVPN.split-tunnel";
#endif
}

QString QStr(NSString *value)
{
    return value != nil ? QString::fromNSString(value) : QStringLiteral("(nil)");
}

QString ErrorDump(NSError *error)
{
    if (error == nil) {
        return QStringLiteral("nil");
    }
    return QStringLiteral("%1 code=%2 desc=%3 userInfo=%4")
        .arg(QStr(error.domain))
        .arg(error.code)
        .arg(QStr(error.localizedDescription))
        .arg(QStr(error.userInfo.description));
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

const char *RequestResultName(OSSystemExtensionRequestResult result)
{
    switch (result) {
    case OSSystemExtensionRequestCompleted: return "completed";
    case OSSystemExtensionRequestWillCompleteAfterReboot: return "willCompleteAfterReboot";
    default: return "unknown";
    }
}

void LogApps(const char *where, const QVector<amnezia::InstalledAppInfo> &apps)
{
    logger.debug() << where << "apps count=" << apps.size();
    for (int i = 0; i < apps.size(); ++i) {
        const auto &app = apps.at(i);
        logger.debug() << where << "  [" << i << "]"
                       << "name=" << app.appName
                       << "bundleId=" << app.packageName
                       << "path=" << app.appPath;
    }
}

void LogHostBundle()
{
    NSBundle *bundle = [NSBundle mainBundle];
    logger.debug() << "host bundle path=" << QStr(bundle.bundlePath) << "id=" << QStr(bundle.bundleIdentifier);
    logger.debug() << "host NSSystemExtensionUsageDescription="
                   << QStr([bundle objectForInfoDictionaryKey:@"NSSystemExtensionUsageDescription"]);

    NSString *sysexDir = [bundle.bundlePath stringByAppendingPathComponent:@"Contents/Library/SystemExtensions"];
    NSArray<NSString *> *entries = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:sysexDir error:nil];
    if (entries == nil) {
        logger.error() << "host has no Contents/Library/SystemExtensions directory at" << QStr(sysexDir)
                       << "- activation will fail with 'extension not found'";
        return;
    }
    logger.debug() << "SystemExtensions dir=" << QStr(sysexDir) << "entries=" << entries.count;
    for (NSString *entry in entries) {
        NSString *path = [sysexDir stringByAppendingPathComponent:entry];
        NSBundle *sysex = [NSBundle bundleWithPath:path];
        logger.debug() << "  sysex" << QStr(entry)
                       << "id=" << QStr(sysex.bundleIdentifier)
                       << "version=" << QStr(sysex.infoDictionary[@"CFBundleVersion"])
                       << "shortVersion=" << QStr(sysex.infoDictionary[@"CFBundleShortVersionString"]);
    }
}

NSDictionary *OptionsDictionary(const QByteArray &json)
{
    if (json.isEmpty()) {
        logger.error() << "OptionsDictionary: empty json";
        return @{};
    }
    NSData *data = [NSData dataWithBytes:json.constData() length:static_cast<NSUInteger>(json.size())];
    NSError *error = nil;
    id object = [NSJSONSerialization JSONObjectWithData:data options:0 error:&error];
    if (error != nil) {
        logger.error() << "OptionsDictionary: json parse failed:" << ErrorDump(error);
        return @{};
    }
    if (![object isKindOfClass:[NSDictionary class]]) {
        logger.error() << "OptionsDictionary: json is not a dictionary";
        return @{};
    }
    return object;
}
} // namespace

#pragma mark - OSSystemExtensionRequest delegate

@interface AmneziaSTExtensionDelegate : NSObject <OSSystemExtensionRequestDelegate>
@property (nonatomic, copy) void (^onNeedsApproval)(void);
@property (nonatomic, copy) void (^onFinished)(BOOL ok, NSString *message);
@end

@implementation AmneziaSTExtensionDelegate

- (OSSystemExtensionReplacementAction)request:(OSSystemExtensionRequest *)request
                  actionForReplacingExtension:(OSSystemExtensionProperties *)existing
                                withExtension:(OSSystemExtensionProperties *)ext
{
    (void)request;
    logger.info() << "sysex: replacing" << QStr(existing.bundleIdentifier) << "v" << QStr(existing.bundleVersion)
                  << "with" << QStr(ext.bundleIdentifier) << "v" << QStr(ext.bundleVersion);
    return OSSystemExtensionReplacementActionReplace;
}

- (void)requestNeedsUserApproval:(OSSystemExtensionRequest *)request
{
    (void)request;
    logger.info() << "sysex: waiting for user approval in System Settings";
    if (self.onNeedsApproval) {
        self.onNeedsApproval();
    }
}

- (void)request:(OSSystemExtensionRequest *)request didFailWithError:(NSError *)error
{
    (void)request;
    logger.error() << "sysex: request failed:" << ErrorDump(error);
    if (self.onFinished) {
        self.onFinished(NO, error.localizedDescription);
    }
}

- (void)request:(OSSystemExtensionRequest *)request didFinishWithResult:(OSSystemExtensionRequestResult)result
{
    (void)request;
    logger.info() << "sysex: request finished result=" << RequestResultName(result) << "(" << (int)result << ")";
    if (result == OSSystemExtensionRequestWillCompleteAfterReboot) {
        logger.info() << "sysex: the extension will only become active after a reboot";
    }
    if (self.onFinished) {
        self.onFinished(YES, nil);
    }
}

@end

static AmneziaSTExtensionDelegate *g_activationDelegate = nil;
static AmneziaSTExtensionDelegate *g_deactivationDelegate = nil;

#pragma mark - MacOSSplitTunnelManager

MacOSSplitTunnelManager *MacOSSplitTunnelManager::instance()
{
    static MacOSSplitTunnelManager *s_instance = new MacOSSplitTunnelManager(nullptr);
    return s_instance;
}

MacOSSplitTunnelManager::MacOSSplitTunnelManager(QObject *parent) : QObject(parent)
{
    logger.info() << "created, provider bundle id =" << QStr(ProviderBundleId());
}

void MacOSSplitTunnelManager::postError(const QString &message)
{
    logger.error() << "reporting error to UI:" << message;
    QMetaObject::invokeMethod(this, [this, message]() { emit errorOccurred(message); }, Qt::QueuedConnection);
}

void MacOSSplitTunnelManager::postNeedsApproval()
{
    QMetaObject::invokeMethod(this, [this]() { emit needsUserApproval(); }, Qt::QueuedConnection);
}

void MacOSSplitTunnelManager::postActivated()
{
    QMetaObject::invokeMethod(this, [this]() { emit extensionActivated(); }, Qt::QueuedConnection);
}

QByteArray MacOSSplitTunnelManager::optionsJson(const QVector<amnezia::InstalledAppInfo> &apps,
                                                const QString &vpnServer) const
{
    QJsonArray appsJson;
    for (const auto &app : apps) {
        QJsonObject entry;
        entry.insert(QStringLiteral("bundleId"), app.packageName);
        entry.insert(QStringLiteral("path"), app.appPath);
        appsJson.append(entry);
    }

    QJsonObject root;
    // Only the exclude mode is implemented; the extension refuses to claim any
    // flow for any other value instead of silently excluding.
    root.insert(QStringLiteral("mode"), QStringLiteral("except"));
    root.insert(QStringLiteral("apps"), appsJson);
    root.insert(QStringLiteral("vpnServer"), vpnServer);
    const QByteArray json = QJsonDocument(root).toJson(QJsonDocument::Compact);
    logger.debug() << "options json =" << QString::fromUtf8(json);
    return json;
}

void MacOSSplitTunnelManager::activateExtension()
{
    if (@available(macOS 11.0, *)) {
        logger.info() << "activateExtension: requested=" << m_activationRequested
                      << "activated=" << m_extensionActivated
                      << "provider=" << QStr(ProviderBundleId());
        LogHostBundle();

        if (m_activationRequested) {
            logger.debug() << "activateExtension: a request is already in flight or completed";
            return;
        }
        m_activationRequested = true;

        g_activationDelegate = [[AmneziaSTExtensionDelegate alloc] init];
        g_activationDelegate.onNeedsApproval = ^{
            this->postNeedsApproval();
        };
        g_activationDelegate.onFinished = ^(BOOL ok, NSString *message) {
            if (ok) {
                QMetaObject::invokeMethod(this, [this]() { onExtensionActivated(); }, Qt::QueuedConnection);
            } else {
                this->m_activationRequested = false;
                this->postError(QStr(message));
            }
        };

        OSSystemExtensionRequest *request =
            [OSSystemExtensionRequest activationRequestForExtension:ProviderBundleId()
                                                              queue:dispatch_get_main_queue()];
        request.delegate = g_activationDelegate;
        logger.info() << "activateExtension: submitting activation request";
        [[OSSystemExtensionManager sharedManager] submitRequest:request];
    } else {
        logger.error() << "activateExtension: macOS 11.0 or newer is required";
        postError(tr("Split tunneling requires macOS 11 or newer"));
    }
}

void MacOSSplitTunnelManager::onExtensionActivated()
{
    m_extensionActivated = true;
    logger.info() << "extension is registered; desired.valid=" << m_desired.valid
                  << "desired.shouldRun=" << m_desired.shouldRun;
    postActivated();

    // A first-run activation completes long after reconcile() asked for the
    // proxy. Start it now, otherwise nothing happens until the user toggles
    // something or reconnects.
    if (m_desired.valid && m_desired.shouldRun) {
        logger.info() << "starting the proxy that was pending on approval";
        startProxy(m_desired.apps, m_desired.vpnServer);
    }
}

void MacOSSplitTunnelManager::deactivateExtension()
{
    if (@available(macOS 11.0, *)) {
        logger.info() << "deactivateExtension: submitting deactivation request for" << QStr(ProviderBundleId());

        g_deactivationDelegate = [[AmneziaSTExtensionDelegate alloc] init];
        g_deactivationDelegate.onNeedsApproval = ^{
            logger.info() << "deactivateExtension: waiting for user approval";
        };
        g_deactivationDelegate.onFinished = ^(BOOL ok, NSString *message) {
            if (ok) {
                logger.info() << "deactivateExtension: done";
            } else {
                logger.error() << "deactivateExtension failed:" << QStr(message);
            }
        };

        OSSystemExtensionRequest *request =
            [OSSystemExtensionRequest deactivationRequestForExtension:ProviderBundleId()
                                                                queue:dispatch_get_main_queue()];
        request.delegate = g_deactivationDelegate;
        [[OSSystemExtensionManager sharedManager] submitRequest:request];

        m_activationRequested = false;
        m_extensionActivated = false;
    }
}

void MacOSSplitTunnelManager::reconcile(bool vpnConnected, bool splitTunnelEnabled, amnezia::AppsRouteMode mode,
                                        const QVector<amnezia::InstalledAppInfo> &apps, const QString &vpnServer)
{
    const bool exceptMode = mode == amnezia::AppsRouteMode::VpnAllExceptApps;
    const bool shouldRun = vpnConnected && splitTunnelEnabled && exceptMode && !apps.isEmpty();

    logger.info() << "reconcile: vpnConnected=" << vpnConnected
                  << "splitEnabled=" << splitTunnelEnabled
                  << "mode=" << static_cast<int>(mode) << "(exceptMode=" << exceptMode << ")"
                  << "appsEmpty=" << apps.isEmpty()
                  << "vpnServer=" << (vpnServer.isEmpty() ? QStringLiteral("(empty)") : vpnServer)
                  << "extensionActivated=" << m_extensionActivated
                  << "=> shouldRun=" << shouldRun;
    LogApps("reconcile", apps);

    if (splitTunnelEnabled && !exceptMode) {
        logger.error() << "reconcile: app split tunneling is on but the route mode is" << static_cast<int>(mode)
                       << "- only VpnAllExceptApps is supported on macOS, the proxy stays off";
    }
    if (splitTunnelEnabled && exceptMode && apps.isEmpty()) {
        logger.info() << "reconcile: the app list is empty, nothing to exclude";
    }
    if (shouldRun && vpnServer.isEmpty()) {
        logger.error() << "reconcile: vpnServer is empty - the VPN endpoint will not be excluded from the "
                          "proxy rules (expected for API configs, where credentials are not stored)";
    }

    m_desired.valid = true;
    m_desired.shouldRun = shouldRun;
    m_desired.apps = apps;
    m_desired.vpnServer = vpnServer;

    if (!shouldRun) {
        stopProxy();
        return;
    }

    activateExtension();

    if (!m_extensionActivated) {
        logger.info() << "reconcile: extension is not registered yet, the proxy will start once it is";
        return;
    }

    startProxy(apps, vpnServer);
}

void MacOSSplitTunnelManager::disableFeature()
{
    logger.info() << "disableFeature: stopping the proxy, removing the configuration and unregistering "
                     "the extension";
    m_desired = DesiredState {};
    m_appliedOptions.clear();
    stopProxy();
    removeConfiguration();
    deactivateExtension();
}

void MacOSSplitTunnelManager::startProxy(const QVector<amnezia::InstalledAppInfo> &apps, const QString &vpnServer)
{
    if (@available(macOS 11.0, *)) {
        const QByteArray json = optionsJson(apps, vpnServer);
        NSDictionary *options = OptionsDictionary(json);
        NSString *bundleId = ProviderBundleId();
        NSData *messageData = [NSData dataWithBytes:json.constData() length:static_cast<NSUInteger>(json.size())];
        const QByteArray previousOptions = m_appliedOptions;
        m_appliedOptions = json;

        logger.info() << "startProxy: provider=" << QStr(bundleId) << "apps=" << apps.size()
                      << "optionsChanged=" << (previousOptions != json);

        [NETransparentProxyManager loadAllFromPreferencesWithCompletionHandler:^(NSArray<NETransparentProxyManager *> *managers,
                                                                                NSError *loadError) {
            if (loadError != nil) {
                logger.error() << "startProxy: loadAllFromPreferences failed:" << ErrorDump(loadError);
                this->postError(QStr(loadError.localizedDescription));
                return;
            }

            logger.debug() << "startProxy: found" << (managers != nil ? (int)managers.count : 0) << "NE configurations";
            NETransparentProxyManager *manager = nil;
            int index = 0;
            for (NETransparentProxyManager *item in managers) {
                NETunnelProviderProtocol *proto = (NETunnelProviderProtocol *)item.protocolConfiguration;
                const bool isTunnelProto = [proto isKindOfClass:[NETunnelProviderProtocol class]];
                logger.debug() << "  [" << index++ << "]"
                               << "enabled=" << item.enabled
                               << "status=" << VpnStatusName(item.connection.status)
                               << "provider=" << (isTunnelProto ? QStr(proto.providerBundleIdentifier)
                                                                : QStringLiteral("<not-tunnel-proto>"))
                               << "desc=" << QStr(item.localizedDescription);
                if (isTunnelProto && [proto.providerBundleIdentifier isEqualToString:bundleId]) {
                    manager = item;
                }
            }

            if (manager == nil) {
                logger.info() << "startProxy: creating a new NETransparentProxyManager";
                manager = [[NETransparentProxyManager alloc] init];
            } else {
                logger.debug() << "startProxy: reusing the existing NETransparentProxyManager";
            }

            const NEVPNStatus statusBefore = manager.connection.status;

            // Already running: push the new app list through a provider message
            // instead of restarting the tunnel.
            if (statusBefore == NEVPNStatusConnected && previousOptions == json) {
                logger.info() << "startProxy: proxy is already running with the same options, nothing to do";
                return;
            }
            if (statusBefore == NEVPNStatusConnected) {
                logger.info() << "startProxy: proxy is running, pushing the new options as a provider message";
                NETunnelProviderSession *session = (NETunnelProviderSession *)manager.connection;
                if ([session isKindOfClass:[NETunnelProviderSession class]]) {
                    NSError *sendError = nil;
                    const BOOL sent = [session sendProviderMessage:messageData
                                                       returnError:&sendError
                                                   responseHandler:^(NSData *response) {
                                                       logger.debug() << "startProxy: provider replied"
                                                                      << (response != nil ? (int)response.length : -1)
                                                                      << "bytes";
                                                   }];
                    if (sent) {
                        return;
                    }
                    logger.error() << "startProxy: sendProviderMessage failed, restarting the proxy:"
                                   << ErrorDump(sendError);
                } else {
                    logger.error() << "startProxy: connection is not an NETunnelProviderSession, restarting";
                }
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
                    logger.error() << "startProxy: saveToPreferences failed:" << ErrorDump(saveError);
                    this->postError(QStr(saveError.localizedDescription));
                    return;
                }
                logger.debug() << "startProxy: saveToPreferences ok";

                // Reload so the connection object refers to the saved configuration.
                [manager loadFromPreferencesWithCompletionHandler:^(NSError *reloadError) {
                    if (reloadError != nil) {
                        logger.error() << "startProxy: reload failed:" << ErrorDump(reloadError);
                        this->postError(QStr(reloadError.localizedDescription));
                        return;
                    }
                    logger.debug() << "startProxy: reload ok, status=" << VpnStatusName(manager.connection.status);

                    if (manager.connection.status == NEVPNStatusConnected
                        || manager.connection.status == NEVPNStatusConnecting) {
                        logger.info() << "startProxy: already"
                                      << VpnStatusName(manager.connection.status) << ", not starting again";
                        return;
                    }

                    NSError *startError = nil;
                    const BOOL started = [manager.connection startVPNTunnelWithOptions:options
                                                                        andReturnError:&startError];
                    logger.info() << "startProxy: startVPNTunnel started=" << started
                                  << "status=" << VpnStatusName(manager.connection.status)
                                  << "error=" << ErrorDump(startError);
                    if (!started) {
                        this->postError(startError != nil ? QStr(startError.localizedDescription)
                                                          : QStringLiteral("startVPNTunnel returned NO"));
                    }
                }];
            }];
        }];
    } else {
        logger.error() << "startProxy: macOS 11.0 or newer is required";
    }
}

void MacOSSplitTunnelManager::stopProxy()
{
    if (@available(macOS 11.0, *)) {
        NSString *bundleId = ProviderBundleId();
        logger.info() << "stopProxy: provider=" << QStr(bundleId);
        m_appliedOptions.clear();

        [NETransparentProxyManager loadAllFromPreferencesWithCompletionHandler:^(NSArray<NETransparentProxyManager *> *managers,
                                                                                NSError *loadError) {
            if (loadError != nil) {
                logger.error() << "stopProxy: loadAllFromPreferences failed:" << ErrorDump(loadError);
                return;
            }
            int stopped = 0;
            for (NETransparentProxyManager *item in managers) {
                NETunnelProviderProtocol *proto = (NETunnelProviderProtocol *)item.protocolConfiguration;
                if ([proto isKindOfClass:[NETunnelProviderProtocol class]]
                    && [proto.providerBundleIdentifier isEqualToString:bundleId]) {
                    logger.info() << "stopProxy: stopping tunnel, status was"
                                  << VpnStatusName(item.connection.status);
                    [item.connection stopVPNTunnel];
                    ++stopped;
                }
            }
            logger.debug() << "stopProxy: stopped" << stopped << "configuration(s)";
        }];
    }
}

void MacOSSplitTunnelManager::removeConfiguration()
{
    if (@available(macOS 11.0, *)) {
        NSString *bundleId = ProviderBundleId();
        logger.info() << "removeConfiguration: provider=" << QStr(bundleId);

        [NETransparentProxyManager loadAllFromPreferencesWithCompletionHandler:^(NSArray<NETransparentProxyManager *> *managers,
                                                                                NSError *loadError) {
            if (loadError != nil) {
                logger.error() << "removeConfiguration: loadAllFromPreferences failed:" << ErrorDump(loadError);
                return;
            }
            for (NETransparentProxyManager *item in managers) {
                NETunnelProviderProtocol *proto = (NETunnelProviderProtocol *)item.protocolConfiguration;
                if (![proto isKindOfClass:[NETunnelProviderProtocol class]]
                    || ![proto.providerBundleIdentifier isEqualToString:bundleId]) {
                    continue;
                }
                [item removeFromPreferencesWithCompletionHandler:^(NSError *removeError) {
                    if (removeError != nil) {
                        logger.error() << "removeConfiguration failed:" << ErrorDump(removeError);
                    } else {
                        logger.info() << "removeConfiguration: removed";
                    }
                }];
            }
        }];
    }
}
