#include "macosSplitTunnelManager.h"

#import <Foundation/Foundation.h>
#import <NetworkExtension/NetworkExtension.h>
#import <SystemExtensions/SystemExtensions.h>

#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>
#include <QStandardPaths>

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

/*! Where the extension writes its own log file.
 *
 *  The extension runs as root, so NSHomeDirectory() there is /var/root and it
 *  cannot find the user's folder on its own - the path has to travel in the
 *  start options. We create the directory here, from the user's process, so it
 *  ends up owned by the user rather than by root. */
QString SplitTunnelLogDirectory()
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (base.isEmpty()) {
        logger.error() << "cannot resolve the Documents directory, the extension will not write a log file";
        return {};
    }
    const QString dir = base + QStringLiteral("/AmneziaVPN");
    if (!QDir().mkpath(dir)) {
        logger.error() << "cannot create the log directory" << dir;
        return {};
    }
    return dir;
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
@property (nonatomic, copy) void (^onProperties)(BOOL found, BOOL enabled, BOOL awaitingApproval);
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
    // A properties request for an extension macOS does not know about fails
    // instead of returning an empty list; that is the "not installed" answer.
    if (self.onProperties) {
        self.onProperties(NO, NO, NO);
        return;
    }
    if (self.onFinished) {
        self.onFinished(NO, error.localizedDescription);
    }
}

- (void)request:(OSSystemExtensionRequest *)request
    foundProperties:(NSArray<OSSystemExtensionProperties *> *)properties API_AVAILABLE(macos(12.0))
{
    (void)request;
    BOOL found = NO;
    BOOL enabled = NO;
    BOOL awaiting = NO;
    int uninstalling = 0;
    for (OSSystemExtensionProperties *item in properties) {
        if (item.isUninstalling) {
            // Records left by previous deactivations. macOS only drops them on
            // reboot, and they pile up one per off/on cycle.
            ++uninstalling;
            continue;
        }
        found = YES;
        enabled = enabled || item.isEnabled;
        awaiting = awaiting || item.isAwaitingUserApproval;
        logger.info() << "sysex properties: v" << QStr(item.bundleVersion) << "enabled=" << (item.isEnabled ? 1 : 0)
                      << "awaitingApproval=" << (item.isAwaitingUserApproval ? 1 : 0);
    }
    if (uninstalling > 0) {
        logger.warning() << "sysex properties:" << uninstalling
                         << "copies are waiting to uninstall on reboot; macOS may refuse to show the "
                            "approval row until the machine is restarted";
    }
    if (!found) {
        logger.info() << "sysex properties: the extension is not installed";
    }
    if (self.onProperties) {
        self.onProperties(found, enabled, awaiting);
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
static AmneziaSTExtensionDelegate *g_propertiesDelegate = nil;

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
    // Ask the system what it already has before anything decides to install.
    refreshExtensionState();
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
                                                const QString &vpnServer,
                                                amnezia::AppsRouteMode mode) const
{
    QJsonArray appsJson;
    for (const auto &app : apps) {
        QJsonObject entry;
        entry.insert(QStringLiteral("bundleId"), app.packageName);
        entry.insert(QStringLiteral("path"), app.appPath);
        appsJson.append(entry);
    }

    QJsonObject root;
    root.insert(QStringLiteral("mode"),
                mode == amnezia::AppsRouteMode::VpnOnlyForwardApps ? QStringLiteral("only")
                                                                   : QStringLiteral("except"));
    root.insert(QStringLiteral("apps"), appsJson);
    root.insert(QStringLiteral("vpnServer"), vpnServer);

    // Self-exclusion. In "only" mode the extension claims every flow that is not
    // listed, so it must be able to recognise our own processes - the app, this
    // extension, and the helpers shipped inside the bundle (AmneziaVPN-service,
    // tun2socks, amneziawg-go, openvpn) - or it would relay its own sockets.
    NSBundle *bundle = [NSBundle mainBundle];
    root.insert(QStringLiteral("selfBundleId"), QStr(bundle.bundleIdentifier));
    root.insert(QStringLiteral("selfAppPath"), QStr(bundle.bundlePath));

    // File logging for the extension. It writes into
    // <logDir>/AmneziaVPNSplitTunnel_root/AmneziaVPNSplitTunnel.log, next to the app
    // and service logs.
    const QString logDir = SplitTunnelLogDirectory();
    root.insert(QStringLiteral("logDir"), logDir);
#ifdef QT_NO_DEBUG
    // Release: lifecycle only. Flip to true to get the per-read chatter.
    root.insert(QStringLiteral("logDebug"), false);
#else
    root.insert(QStringLiteral("logDebug"), true);
#endif

    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

#pragma mark - System extension registration

void MacOSSplitTunnelManager::refreshExtensionState()
{
    if (@available(macOS 12.0, *)) {
        g_propertiesDelegate = [[AmneziaSTExtensionDelegate alloc] init];
        g_propertiesDelegate.onProperties = ^(BOOL found, BOOL enabled, BOOL awaiting) {
            const bool installedCopy = found ? true : false;
            const bool enabledCopy = enabled ? true : false;
            const bool awaitingCopy = awaiting ? true : false;
            QMetaObject::invokeMethod(
                this,
                [this, installedCopy, enabledCopy, awaitingCopy]() {
                    onExtensionStateKnown(installedCopy, enabledCopy, awaitingCopy);
                },
                Qt::QueuedConnection);
        };

        OSSystemExtensionRequest *request =
            [OSSystemExtensionRequest propertiesRequestForExtension:ProviderBundleId()
                                                              queue:dispatch_get_main_queue()];
        request.delegate = g_propertiesDelegate;
        [[OSSystemExtensionManager sharedManager] submitRequest:request];
    } else {
        // Before macOS 12 there is no way to ask; assume whatever this process
        // did itself is the truth.
        onExtensionStateKnown(m_extensionActivated, m_extensionActivated, false);
    }
}

void MacOSSplitTunnelManager::onExtensionStateKnown(bool installed, bool enabled, bool awaitingApproval)
{
    const bool changed = installed != m_extensionInstalled || enabled != m_extensionEnabled
                         || awaitingApproval != m_extensionAwaitingApproval;
    m_extensionInstalled = installed;
    m_extensionEnabled = enabled;
    m_extensionAwaitingApproval = awaitingApproval;

    // The system is the authority: an extension macOS still has registered does
    // not need another activation request, and one it dropped does.
    m_extensionActivated = installed && enabled;
    if (!installed) {
        m_activationRequested = false;
    }

    logger.info() << "extension state: installed=" << installed << "enabled=" << enabled
                  << "awaitingApproval=" << awaitingApproval;
    if (changed) {
        emit extensionStateChanged(installed, enabled, awaitingApproval);
    }
}

void MacOSSplitTunnelManager::activateExtension(bool userInitiated)
{
    if (@available(macOS 11.0, *)) {
        if (m_extensionInstalled && m_extensionEnabled) {
            logger.info() << "activateExtension: already installed and enabled, nothing to do";
            m_extensionActivated = true;
            return;
        }
        if (m_activationRequested) {
            logger.debug() << "activateExtension: a request is already in flight";
            return;
        }
        if (userInitiated && m_extensionAwaitingApproval) {
            // An approval the user never answered keeps the record in
            // "activated waiting for user" forever, and a fresh activation
            // request on top of it produces no prompt at all. Removing the
            // record first is what "systemextensionsctl uninstall" does by
            // hand, and it makes macOS offer the install again.
            logger.info() << "activateExtension: an unanswered approval is pending; removing it first so "
                             "macOS offers the install again";
            deactivateExtension(/*reactivateAfterwards=*/true);
            return;
        }
        if (m_extensionInstalled && !m_extensionEnabled) {
            logger.info() << "activateExtension: installed but switched off in System Settings; "
                             "re-activating so macOS re-enables it";
        }
        logger.info() << "activateExtension: submitting request for" << QStr(ProviderBundleId());
        LogHostBundle();
        m_activationRequested = true;

        g_activationDelegate = [[AmneziaSTExtensionDelegate alloc] init];
        g_activationDelegate.onNeedsApproval = ^{
            this->postNeedsApproval();
        };
        g_activationDelegate.onFinished = ^(BOOL ok, NSString *message) {
            this->m_activationRequested = false;
            if (ok) {
                QMetaObject::invokeMethod(this, [this]() { onExtensionActivated(); }, Qt::QueuedConnection);
            } else {
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
    // The request only says macOS accepted it; the toggle state comes from the
    // properties request.
    refreshExtensionState();

    // A first-run activation completes long after reconcile() asked for the
    // proxy, so start it now instead of waiting for the next state change.
    if (m_desired.valid && m_desired.shouldRun) {
        logger.info() << "starting the proxy that was pending on approval";
        startProxy();
    }
}

void MacOSSplitTunnelManager::deactivateExtension(bool reactivateAfterwards)
{
    if (@available(macOS 11.0, *)) {
        // macOS knows about the extension even when this process never
        // registered it - a record left pending by an earlier run is exactly
        // the case we have to clear - so go by what the properties request
        // reported, not by what this process did.
        if (!m_activationRequested && !m_extensionActivated && !m_extensionInstalled) {
            logger.info() << "deactivateExtension: nothing registered, skipping";
            if (reactivateAfterwards) {
                activateExtension();
            }
            return;
        }

        logger.info() << "deactivateExtension: submitting request for" << QStr(ProviderBundleId())
                      << "reactivateAfterwards=" << reactivateAfterwards;
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
            if (!reactivateAfterwards) {
                return;
            }
            // The record is gone, so the next activation request produces a
            // fresh install prompt. Back on the Qt thread: the state this
            // touches belongs to it.
            QMetaObject::invokeMethod(
                this,
                [this]() {
                    m_extensionInstalled = false;
                    m_extensionEnabled = false;
                    m_extensionAwaitingApproval = false;
                    logger.info() << "deactivateExtension: re-submitting the activation request";
                    activateExtension();
                },
                Qt::QueuedConnection);
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
    const bool modeSupported = mode == amnezia::AppsRouteMode::VpnAllExceptApps
        || mode == amnezia::AppsRouteMode::VpnOnlyForwardApps;
    const bool shouldRun = vpnConnected && splitTunnelEnabled && modeSupported && !apps.isEmpty();
    const QByteArray options = optionsJson(apps, vpnServer, mode);

    const bool desiredChanged = !m_desired.valid || m_desired.shouldRun != shouldRun
        || options != optionsJson(m_desired.apps, m_desired.vpnServer, m_desired.mode);

    m_desired.valid = true;
    m_desired.shouldRun = shouldRun;
    m_desired.mode = mode;
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
                  << "mode=" << (mode == amnezia::AppsRouteMode::VpnOnlyForwardApps ? "only" : "except")
                  << "apps=" << apps.size()
                  << "extensionActivated=" << m_extensionActivated
                  << "proxyStatus=" << (m_proxyStatus < 0 ? "unknown" : VpnStatusName(static_cast<NEVPNStatus>(m_proxyStatus)))
                  << "=> shouldRun=" << shouldRun;
    LogApps("reconcile", apps);

    if (splitTunnelEnabled && !modeSupported) {
        logger.error() << "reconcile: unsupported route mode" << static_cast<int>(mode)
                       << "- the proxy stays off";
    }
    if (shouldRun && mode == amnezia::AppsRouteMode::VpnOnlyForwardApps) {
        logger.info() << "reconcile: include mode claims every flow that is not listed, so all traffic "
                         "except the listed apps is relayed through the extension";
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
        const QByteArray json = optionsJson(m_desired.apps, m_desired.vpnServer, m_desired.mode);
        const bool optionsChanged = (json != m_appliedOptions);
        NSDictionary *options = OptionsDictionary(json);
        NSData *messageData = [NSData dataWithBytes:json.constData() length:static_cast<NSUInteger>(json.size())];
        NSString *bundleId = ProviderBundleId();

        logger.info() << "startProxy: apps=" << m_desired.apps.size() << "optionsChanged=" << optionsChanged
                      << "extension log dir=" << SplitTunnelLogDirectory() + QStringLiteral("/AmneziaVPNSplitTunnel_root");
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
    logger.info() << "disableFeature: stopping the proxy and disabling the configuration "
                     "(the extension stays installed)";
    m_tearingDown = true;
    m_desired = DesiredState {};
    m_appliedOptions.clear();

    if (@available(macOS 11.0, *)) {
        if (g_proxyManager == nil && m_proxyStatus < 0) {
            // Nothing was ever configured in this process; there may still be a
            // saved configuration from an earlier run, so load it and turn it
            // off rather than assuming there is nothing to do.
            logger.debug() << "disableFeature: nothing running here, disabling any saved configuration";
        }

        WithProxyManager(^(NETransparentProxyManager *manager, NSError *loadError) {
            if (manager == nil) {
                logger.debug() << "disableFeature: no saved configuration:" << ErrorDump(loadError);
                this->m_tearingDown = false;
                this->refreshExtensionState();
                return;
            }
            if (manager.connection.status != NEVPNStatusDisconnected
                && manager.connection.status != NEVPNStatusInvalid) {
                logger.info() << "disableFeature: stopping the proxy, status was"
                              << VpnStatusName(manager.connection.status);
                [manager.connection stopVPNTunnel];
            }
            this->setConfigurationEnabled(false, ^{
                this->m_tearingDown = false;
                this->refreshExtensionState();
            });
        });
    } else {
        m_tearingDown = false;
    }
}

void MacOSSplitTunnelManager::uninstallFeature()
{
    logger.info() << "uninstallFeature: tearing down (stop -> remove configuration -> unregister)";
    m_tearingDown = true;
    m_desired = DesiredState {};
    m_appliedOptions.clear();

    if (@available(macOS 11.0, *)) {
        // Sequential on purpose: removing the configuration while the tunnel is
        // still stopping, or unregistering before the configuration is gone,
        // leaves entries behind in System Settings.
        if (g_proxyManager == nil && m_proxyStatus < 0) {
            logger.debug() << "uninstallFeature: nothing was running";
            deactivateExtension();
            m_tearingDown = false;
            return;
        }

        WithProxyManager(^(NETransparentProxyManager *manager, NSError *loadError) {
            if (manager != nil && manager.connection.status != NEVPNStatusDisconnected
                && manager.connection.status != NEVPNStatusInvalid) {
                logger.info() << "uninstallFeature: stopping the proxy, status was"
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

void MacOSSplitTunnelManager::setConfigurationEnabled(bool enabled, void (^completion)(void))
{
    if (@available(macOS 11.0, *)) {
        if (g_proxyManager == nil) {
            logger.debug() << "setConfigurationEnabled: nothing loaded";
            if (completion) {
                completion();
            }
            return;
        }
        if (g_proxyManager.protocolConfiguration == nil) {
            // WithProxyManager hands out a blank manager when preferences hold
            // no configuration for us. Saving that one fails with "Missing
            // protocol", and there is nothing to disable anyway.
            logger.debug() << "setConfigurationEnabled: no saved configuration, nothing to disable";
            if (completion) {
                completion();
            }
            return;
        }
        if (g_proxyManager.enabled == (enabled ? YES : NO)) {
            logger.debug() << "setConfigurationEnabled: already" << enabled;
            if (completion) {
                completion();
            }
            return;
        }

        logger.info() << "setConfigurationEnabled:" << enabled;
        g_proxyManager.enabled = enabled ? YES : NO;
        [g_proxyManager saveToPreferencesWithCompletionHandler:^(NSError *saveError) {
            if (saveError != nil) {
                logger.error() << "setConfigurationEnabled failed:" << ErrorDump(saveError);
            } else {
                logger.info() << "setConfigurationEnabled: saved";
            }
            if (completion) {
                completion();
            }
        }];
    } else if (completion) {
        completion();
    }
}
