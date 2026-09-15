#include "macosSplitTunnelManager.h"

#import <Foundation/Foundation.h>
#import <NetworkExtension/NetworkExtension.h>
#import <SystemExtensions/SystemExtensions.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>

#include "logger.h"

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

QString QStr(NSString *value)
{
    return value != nil ? QString::fromNSString(value) : QStringLiteral("(nil)");
}

QString ErrorDump(NSError *error)
{
    if (error == nil) {
        return QStringLiteral("nil");
    }
    return QStringLiteral("%1 code=%2 desc=%3")
        .arg(QStr(error.domain))
        .arg(error.code)
        .arg(QStr(error.localizedDescription));
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

    NSString *sysexDir = [bundle.bundlePath stringByAppendingPathComponent:@"Contents/Library/SystemExtensions"];
    NSArray<NSString *> *entries = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:sysexDir error:nil];
    if (entries == nil) {
        logger.error() << "host has no Contents/Library/SystemExtensions directory at" << QStr(sysexDir)
                       << "- activation will fail with 'extension not found'";
        return;
    }
    if (entries.count != 1) {
        logger.error() << "expected exactly one system extension bundle, found" << entries.count
                       << "- a stale bundle claiming the same identifier makes sysextd fail";
    }
    for (NSString *entry in entries) {
        NSBundle *sysex = [NSBundle bundleWithPath:[sysexDir stringByAppendingPathComponent:entry]];
        logger.debug() << "  sysex" << QStr(entry)
                       << "id=" << QStr(sysex.bundleIdentifier)
                       << "version=" << QStr(sysex.infoDictionary[@"CFBundleVersion"]);
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
    if (error != nil || ![object isKindOfClass:[NSDictionary class]]) {
        logger.error() << "OptionsDictionary: bad json:" << ErrorDump(error);
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
    logger.info() << "sysex: request finished result=" << RequestResultName(result);
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

/*! The NE configuration is loaded once and cached, so reconcile() does not pay
 *  for an async loadAllFromPreferences round trip on every connection state
 *  change, and so we can keep a status observer attached to its connection. */
static NETransparentProxyManager *g_proxyManager = nil;
static id g_statusObserver = nil;

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
    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

#pragma mark - System extension registration

void MacOSSplitTunnelManager::activateExtension()
{
    if (@available(macOS 11.0, *)) {
        if (m_activationRequested) {
            logger.debug() << "activateExtension: already requested (activated=" << m_extensionActivated << ")";
            return;
        }
        logger.info() << "activateExtension: submitting request for" << QStr(ProviderBundleId());
        LogHostBundle();
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
        [[OSSystemExtensionManager sharedManager] submitRequest:request];
    } else {
        logger.error() << "activateExtension: macOS 11.0 or newer is required";
        postError(tr("Split tunneling requires macOS 11 or newer"));
    }
}

void MacOSSplitTunnelManager::onExtensionActivated()
{
    m_extensionActivated = true;
    logger.info() << "extension is registered; desired.shouldRun=" << m_desired.shouldRun;
    postActivated();

    // A first-run activation completes long after reconcile() asked for the
    // proxy, so start it now instead of waiting for the next state change.
    if (m_desired.valid && m_desired.shouldRun) {
        logger.info() << "starting the proxy that was pending on approval";
        startProxy();
    }
}

void MacOSSplitTunnelManager::deactivateExtension()
{
    if (@available(macOS 11.0, *)) {
        if (!m_activationRequested && !m_extensionActivated) {
            // Nothing was ever registered from this process; a deactivation
            // request would just come back as ExtensionNotFound (code 4).
            logger.info() << "deactivateExtension: nothing registered, skipping";
            return;
        }

        logger.info() << "deactivateExtension: submitting request for" << QStr(ProviderBundleId());
        g_deactivationDelegate = [[AmneziaSTExtensionDelegate alloc] init];
        g_deactivationDelegate.onNeedsApproval = ^{
            logger.info() << "deactivateExtension: waiting for user approval";
        };
        g_deactivationDelegate.onFinished = ^(BOOL ok, NSString *message) {
            if (ok) {
                logger.info() << "deactivateExtension: done";
            } else {
                // Not surfaced to the UI: the user asked to turn the feature
                // off, and a failure to unregister does not affect them.
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

#pragma mark - NE configuration cache

namespace {
/*! Loads (or creates) the NETransparentProxyManager for our provider exactly
 *  once, caches it in g_proxyManager and calls back on the main queue. */
void WithProxyManager(void (^completion)(NETransparentProxyManager *manager, NSError *error))
{
    if (g_proxyManager != nil) {
        completion(g_proxyManager, nil);
        return;
    }

    NSString *bundleId = ProviderBundleId();
    [NETransparentProxyManager loadAllFromPreferencesWithCompletionHandler:^(NSArray<NETransparentProxyManager *> *managers,
                                                                            NSError *loadError) {
        if (loadError != nil) {
            logger.error() << "loadAllFromPreferences failed:" << ErrorDump(loadError);
            completion(nil, loadError);
            return;
        }

        logger.debug() << "loadAllFromPreferences:" << (managers != nil ? (int)managers.count : 0) << "configuration(s)";
        for (NETransparentProxyManager *item in managers) {
            NETunnelProviderProtocol *proto = (NETunnelProviderProtocol *)item.protocolConfiguration;
            if ([proto isKindOfClass:[NETunnelProviderProtocol class]]
                && [proto.providerBundleIdentifier isEqualToString:bundleId]) {
                logger.debug() << "reusing the existing configuration, status="
                               << VpnStatusName(item.connection.status);
                g_proxyManager = item;
                break;
            }
        }
        if (g_proxyManager == nil) {
            logger.info() << "creating a new NETransparentProxyManager";
            g_proxyManager = [[NETransparentProxyManager alloc] init];
        }
        completion(g_proxyManager, nil);
    }];
}
} // namespace

void MacOSSplitTunnelManager::onProxyStatusChanged(int status)
{
    const NEVPNStatus previous = static_cast<NEVPNStatus>(m_proxyStatus);
    const NEVPNStatus current = static_cast<NEVPNStatus>(status);
    if (m_proxyStatus == status) {
        return;
    }
    m_proxyStatus = status;
    logger.info() << "proxy status" << (m_proxyStatus < 0 ? "(unknown)" : VpnStatusName(previous))
                  << "->" << VpnStatusName(current);

    if (current == NEVPNStatusConnected) {
        QMetaObject::invokeMethod(this, [this]() { emit proxyStarted(); }, Qt::QueuedConnection);
        return;
    }

    if (current == NEVPNStatusDisconnected) {
        QMetaObject::invokeMethod(this, [this]() { emit proxyStopped(); }, Qt::QueuedConnection);
        if (m_desired.valid && m_desired.shouldRun && !m_tearingDown) {
            // We asked for it to run and it went down on its own.
            logger.error() << "the proxy stopped while it was supposed to be running";
            postError(tr("The split tunneling extension stopped unexpectedly"));
        }
    }
}

#pragma mark - Reconcile

void MacOSSplitTunnelManager::reconcile(bool vpnConnected, bool splitTunnelEnabled, amnezia::AppsRouteMode mode,
                                        const QVector<amnezia::InstalledAppInfo> &apps, const QString &vpnServer)
{
    const bool exceptMode = mode == amnezia::AppsRouteMode::VpnAllExceptApps;
    const bool shouldRun = vpnConnected && splitTunnelEnabled && exceptMode && !apps.isEmpty();
    const QByteArray options = optionsJson(apps, vpnServer);

    const bool desiredChanged = !m_desired.valid || m_desired.shouldRun != shouldRun
        || m_desired.vpnServer != vpnServer || m_desired.apps.size() != apps.size()
        || options != optionsJson(m_desired.apps, m_desired.vpnServer);

    m_desired.valid = true;
    m_desired.shouldRun = shouldRun;
    m_desired.apps = apps;
    m_desired.vpnServer = vpnServer;

    const bool proxyRunning = m_proxyStatus == NEVPNStatusConnected || m_proxyStatus == NEVPNStatusConnecting;

    if (!desiredChanged && shouldRun == proxyRunning) {
        logger.debug() << "reconcile: no change (shouldRun=" << shouldRun
                       << "status=" << (m_proxyStatus < 0 ? "unknown" : VpnStatusName(static_cast<NEVPNStatus>(m_proxyStatus)))
                       << ")";
        return;
    }

    logger.info() << "reconcile: vpnConnected=" << vpnConnected
                  << "splitEnabled=" << splitTunnelEnabled
                  << "exceptMode=" << exceptMode
                  << "apps=" << apps.size()
                  << "extensionActivated=" << m_extensionActivated
                  << "proxyStatus=" << (m_proxyStatus < 0 ? "unknown" : VpnStatusName(static_cast<NEVPNStatus>(m_proxyStatus)))
                  << "=> shouldRun=" << shouldRun;
    LogApps("reconcile", apps);

    if (splitTunnelEnabled && !exceptMode) {
        logger.error() << "reconcile: route mode is" << static_cast<int>(mode)
                       << "- only VpnAllExceptApps is supported on macOS, the proxy stays off";
    }
    if (shouldRun && vpnServer.isEmpty() && !m_warnedAboutEmptyServer) {
        m_warnedAboutEmptyServer = true;
        logger.info() << "reconcile: vpnServer is empty, the VPN endpoint will not get its own exclude rule "
                         "(expected for API configs; harmless, the proxy only claims listed apps)";
    }

    if (!shouldRun) {
        stopProxy();
        return;
    }

    activateExtension();
    if (!m_extensionActivated) {
        logger.info() << "reconcile: extension is not registered yet, the proxy will start once it is";
        return;
    }
    startProxy();
}

#pragma mark - Start / stop

void MacOSSplitTunnelManager::startProxy()
{
    if (@available(macOS 11.0, *)) {
        const QByteArray json = optionsJson(m_desired.apps, m_desired.vpnServer);
        const bool optionsChanged = (json != m_appliedOptions);
        NSDictionary *options = OptionsDictionary(json);
        NSData *messageData = [NSData dataWithBytes:json.constData() length:static_cast<NSUInteger>(json.size())];
        NSString *bundleId = ProviderBundleId();

        logger.info() << "startProxy: apps=" << m_desired.apps.size() << "optionsChanged=" << optionsChanged;
        logger.debug() << "startProxy: options =" << QString::fromUtf8(json);

        WithProxyManager(^(NETransparentProxyManager *manager, NSError *loadError) {
            if (manager == nil) {
                this->postError(QStr(loadError.localizedDescription));
                return;
            }

            const NEVPNStatus status = manager.connection.status;
            this->onProxyStatusChanged(static_cast<int>(status));

            // Already up with the same configuration: nothing to do.
            if (status == NEVPNStatusConnected && !optionsChanged) {
                logger.info() << "startProxy: already running with the same options";
                return;
            }

            // Already up, new app list: push it without restarting the tunnel.
            if (status == NEVPNStatusConnected) {
                logger.info() << "startProxy: running, pushing the new app list as a provider message";
                NETunnelProviderSession *session = (NETunnelProviderSession *)manager.connection;
                if ([session isKindOfClass:[NETunnelProviderSession class]]) {
                    NSError *sendError = nil;
                    if ([session sendProviderMessage:messageData returnError:&sendError responseHandler:^(NSData *response) {
                            logger.debug() << "startProxy: provider acknowledged"
                                           << (response != nil ? (int)response.length : -1) << "bytes";
                        }]) {
                        this->m_appliedOptions = json;
                        return;
                    }
                    logger.error() << "startProxy: sendProviderMessage failed, restarting:" << ErrorDump(sendError);
                }
            }

            if (status == NEVPNStatusConnecting) {
                logger.info() << "startProxy: already connecting, letting it finish";
                return;
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

                // Reload so the connection object refers to the saved configuration.
                [manager loadFromPreferencesWithCompletionHandler:^(NSError *reloadError) {
                    if (reloadError != nil) {
                        logger.error() << "startProxy: reload failed:" << ErrorDump(reloadError);
                        this->postError(QStr(reloadError.localizedDescription));
                        return;
                    }

                    // Attach the status observer once, after the connection object is final.
                    if (g_statusObserver == nil) {
                        g_statusObserver = [[NSNotificationCenter defaultCenter]
                            addObserverForName:NEVPNStatusDidChangeNotification
                                        object:manager.connection
                                         queue:[NSOperationQueue mainQueue]
                                    usingBlock:^(NSNotification *note) {
                                        NEVPNConnection *conn = (NEVPNConnection *)note.object;
                                        this->onProxyStatusChanged(static_cast<int>(conn.status));
                                    }];
                        logger.debug() << "startProxy: status observer attached";
                    }

                    NSError *startError = nil;
                    const BOOL started = [manager.connection startVPNTunnelWithOptions:options
                                                                        andReturnError:&startError];
                    this->m_appliedOptions = started ? json : QByteArray();
                    logger.info() << "startProxy: startVPNTunnel started=" << started
                                  << "error=" << ErrorDump(startError);
                    if (!started) {
                        this->postError(startError != nil ? QStr(startError.localizedDescription)
                                                          : QStringLiteral("startVPNTunnel returned NO"));
                    }
                }];
            }];
        });
    } else {
        logger.error() << "startProxy: macOS 11.0 or newer is required";
    }
}

void MacOSSplitTunnelManager::stopProxy()
{
    if (@available(macOS 11.0, *)) {
        // Nothing has ever been loaded or started: no round trip needed.
        if (g_proxyManager == nil && m_proxyStatus < 0 && m_appliedOptions.isEmpty()) {
            logger.debug() << "stopProxy: no configuration in use, nothing to stop";
            return;
        }

        m_appliedOptions.clear();

        WithProxyManager(^(NETransparentProxyManager *manager, NSError *loadError) {
            if (manager == nil) {
                logger.error() << "stopProxy: cannot load the configuration:" << ErrorDump(loadError);
                return;
            }
            const NEVPNStatus status = manager.connection.status;
            this->onProxyStatusChanged(static_cast<int>(status));
            if (status == NEVPNStatusDisconnected || status == NEVPNStatusInvalid) {
                logger.debug() << "stopProxy: already" << VpnStatusName(status);
                return;
            }
            logger.info() << "stopProxy: stopping, status was" << VpnStatusName(status);
            [manager.connection stopVPNTunnel];
        });
    }
}

void MacOSSplitTunnelManager::removeConfiguration(void (^completion)(void))
{
    if (@available(macOS 11.0, *)) {
        if (g_proxyManager == nil) {
            logger.debug() << "removeConfiguration: nothing loaded";
            completion();
            return;
        }

        NETransparentProxyManager *manager = g_proxyManager;
        logger.info() << "removeConfiguration: removing the saved NE configuration";
        [manager removeFromPreferencesWithCompletionHandler:^(NSError *removeError) {
            if (removeError != nil) {
                logger.error() << "removeConfiguration failed:" << ErrorDump(removeError);
            } else {
                logger.info() << "removeConfiguration: removed";
            }
            if (g_statusObserver != nil) {
                [[NSNotificationCenter defaultCenter] removeObserver:g_statusObserver];
                g_statusObserver = nil;
            }
            g_proxyManager = nil;
            this->m_proxyStatus = -1;
            completion();
        }];
    } else {
        completion();
    }
}

void MacOSSplitTunnelManager::disableFeature()
{
    logger.info() << "disableFeature: tearing down (stop -> remove configuration -> unregister)";
    m_tearingDown = true;
    m_desired = DesiredState {};
    m_appliedOptions.clear();

    if (@available(macOS 11.0, *)) {
        // Sequential on purpose: removing the configuration while the tunnel is
        // still stopping, or unregistering before the configuration is gone,
        // leaves entries behind in System Settings.
        if (g_proxyManager == nil && m_proxyStatus < 0) {
            logger.debug() << "disableFeature: nothing was running";
            deactivateExtension();
            m_tearingDown = false;
            return;
        }

        WithProxyManager(^(NETransparentProxyManager *manager, NSError *loadError) {
            if (manager != nil && manager.connection.status != NEVPNStatusDisconnected
                && manager.connection.status != NEVPNStatusInvalid) {
                logger.info() << "disableFeature: stopping the proxy, status was"
                              << VpnStatusName(manager.connection.status);
                [manager.connection stopVPNTunnel];
            }
            this->removeConfiguration(^{
                this->deactivateExtension();
                this->m_tearingDown = false;
            });
        });
    } else {
        m_tearingDown = false;
    }
}
