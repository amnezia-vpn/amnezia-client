#import "TransparentProxyProvider.h"

#import "FlowTCP.h"
#import "FlowUDP.h"
#import "Settings.h"
#import "Utils.h"

#import <Network/Network.h>
#import <os/log.h>
#import <unistd.h>

namespace {
const char *StopReasonName(NEProviderStopReason reason)
{
    switch (reason) {
    case NEProviderStopReasonNone: return "none";
    case NEProviderStopReasonUserInitiated: return "userInitiated";
    case NEProviderStopReasonProviderFailed: return "providerFailed";
    case NEProviderStopReasonNoNetworkAvailable: return "noNetworkAvailable";
    case NEProviderStopReasonUnrecoverableNetworkChange: return "unrecoverableNetworkChange";
    case NEProviderStopReasonProviderDisabled: return "providerDisabled";
    case NEProviderStopReasonAuthenticationCanceled: return "authenticationCanceled";
    case NEProviderStopReasonConfigurationFailed: return "configurationFailed";
    case NEProviderStopReasonIdleTimeout: return "idleTimeout";
    case NEProviderStopReasonConfigurationDisabled: return "configurationDisabled";
    case NEProviderStopReasonConfigurationRemoved: return "configurationRemoved";
    case NEProviderStopReasonSuperceded: return "superceded";
    case NEProviderStopReasonUserLogout: return "userLogout";
    case NEProviderStopReasonUserSwitch: return "userSwitch";
    case NEProviderStopReasonConnectionFailed: return "connectionFailed";
    case NEProviderStopReasonSleep: return "sleep";
    case NEProviderStopReasonAppUpdate: return "appUpdate";
    default: return "unknown";
    }
}

/*! Cap on the "first time we saw this app" log set, so a long-running proxy
 *  cannot grow it without bound. */
const NSUInteger kSeenSigningIdsLimit = 512;
} // namespace

@interface TransparentProxyProvider ()
/*! Written by the path monitor (main queue), read from flow queues. */
@property (atomic) nw_interface_t physicalInterface;
@end

@implementation TransparentProxyProvider {
    STSettings *_settings;
    nw_path_monitor_t _pathMonitor;
    /*! Guarded by STUtils.stateQueue. */
    NSMutableSet<NSString *> *_seenSigningIds;
}

- (instancetype)init
{
    self = [super init];
    if (self) {
        _settings = [[STSettings alloc] init];
        _seenSigningIds = [NSMutableSet set];
        STLogInfo("provider: init pid=%{public}d uid=%{public}d gid=%{public}d",
                  (int)getpid(), (int)getuid(), (int)getgid());
    }
    return self;
}

#pragma mark - Physical interface tracking

- (void)startPathMonitor
{
    if (_pathMonitor != NULL) {
        STLogDebug("provider: path monitor already running");
        return;
    }
    STLogInfo("provider: starting path monitor");
    _pathMonitor = nw_path_monitor_create();
    nw_path_monitor_set_queue(_pathMonitor, dispatch_get_main_queue());

    __weak TransparentProxyProvider *weakSelf = self;
    nw_path_monitor_set_update_handler(_pathMonitor, ^(nw_path_t path) {
        TransparentProxyProvider *strongSelf = weakSelf;
        if (strongSelf == nil) {
            return;
        }
        const nw_path_status_t status = nw_path_get_status(path);
        __block nw_interface_t found = NULL;
        nw_path_enumerate_interfaces(path, ^bool(nw_interface_t interface) {
            const nw_interface_type_t type = nw_interface_get_type(interface);
            STLogDebug("provider: path interface name=%{public}s type=%{public}d",
                       nw_interface_get_name(interface) ?: "?", (int)type);
            if (found == NULL && (type == nw_interface_type_wifi || type == nw_interface_type_wired)) {
                found = interface;
            }
            return true;
        });

        nw_interface_t previous = strongSelf.physicalInterface;
        const char *previousName = previous != NULL ? (nw_interface_get_name(previous) ?: "?") : "(none)";
        const char *foundName = found != NULL ? (nw_interface_get_name(found) ?: "?") : "(none)";
        strongSelf.physicalInterface = found;
        STLogInfo("provider: path update status=%{public}d physical %{public}s -> %{public}s",
                  (int)status, previousName, foundName);
        if (found == NULL) {
            STLogError("provider: no wifi/wired interface available - excluded apps will fall back "
                       "into the tunnel until one appears");
        }
    });
    nw_path_monitor_start(_pathMonitor);
}

- (void)stopPathMonitor
{
    if (_pathMonitor != NULL) {
        STLogInfo("provider: cancelling path monitor");
        nw_path_monitor_cancel(_pathMonitor);
        _pathMonitor = NULL;
    }
    self.physicalInterface = NULL;
}

#pragma mark - Lifecycle

- (void)startProxyWithOptions:(NSDictionary *)options completionHandler:(void (^)(NSError *))completionHandler
{
    STLogInfo("provider: startProxy options=%{public}@", options ?: @{});
    if (options == nil) {
        STLogError("provider: startProxy called without options - the app list is empty, nothing will be excluded");
    }
    [_settings applyDictionary:options source:@"startProxy"];
    [self startPathMonitor];

    NETransparentProxyNetworkSettings *settings =
        [[NETransparentProxyNetworkSettings alloc] initWithTunnelRemoteAddress:@"127.0.0.1"];

    // Wildcard include: both endpoints nil. An explicit ::/0 NWHostEndpoint drops the whole ruleset.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    NENetworkRule *allOutbound = [[NENetworkRule alloc] initWithRemoteNetwork:nil
                                                                 remotePrefix:0
                                                                 localNetwork:nil
                                                                  localPrefix:0
                                                                     protocol:NENetworkRuleProtocolAny
                                                                    direction:NETrafficDirectionOutbound];
#pragma clang diagnostic pop
    settings.includedNetworkRules = @[ allOutbound ];
    settings.excludedNetworkRules = [_settings excludedNetworkRules];

    STLogInfo("provider: applying network settings included=%{public}lu excluded=%{public}lu",
              (unsigned long)settings.includedNetworkRules.count,
              (unsigned long)settings.excludedNetworkRules.count);

    [self setTunnelNetworkSettings:settings completionHandler:^(NSError *error) {
        if (error != nil) {
            STLogError("provider: setTunnelNetworkSettings failed: %{public}@", error);
        } else {
            STLogInfo("provider: setTunnelNetworkSettings ok, proxy is live");
        }
        completionHandler(error);
    }];
}

- (void)stopProxyWithReason:(NEProviderStopReason)reason completionHandler:(void (^)(void))completionHandler
{
    STLogInfo("provider: stopProxy reason=%{public}s (%{public}d)", StopReasonName(reason), (int)reason);
    [self stopPathMonitor];
    dispatch_sync([STUtils stateQueue], ^{
        STLogInfo("provider: clearing %{public}lu remembered signing ids",
                  (unsigned long)self->_seenSigningIds.count);
        [self->_seenSigningIds removeAllObjects];
    });
    completionHandler();
}

- (void)handleAppMessage:(NSData *)messageData completionHandler:(void (^)(NSData *))completionHandler
{
    STLogInfo("provider: handleAppMessage %{public}lu bytes", (unsigned long)messageData.length);
    NSError *error = nil;
    id json = [NSJSONSerialization JSONObjectWithData:messageData options:0 error:&error];
    BOOL ok = NO;
    if (error == nil && [json isKindOfClass:[NSDictionary class]]) {
        ok = [_settings applyDictionary:json source:@"appMessage"];
    } else {
        STLogError("provider: handleAppMessage parse failed: %{public}@", error);
    }

    if (completionHandler != nil) {
        NSDictionary *reply = @{ @"ok" : @(ok) };
        NSData *replyData = [NSJSONSerialization dataWithJSONObject:reply options:0 error:NULL];
        completionHandler(replyData);
    }
}

#pragma mark - Flow handling

- (void)noteFirstSightOfSigningId:(NSString *)sid path:(NSString *)path
{
    if (sid.length == 0) {
        return;
    }
    dispatch_async([STUtils stateQueue], ^{
        if ([self->_seenSigningIds containsObject:sid]) {
            return;
        }
        if (self->_seenSigningIds.count >= kSeenSigningIdsLimit) {
            STLogDebug("provider: seen-ids cache full (%{public}lu), resetting",
                       (unsigned long)self->_seenSigningIds.count);
            [self->_seenSigningIds removeAllObjects];
        }
        [self->_seenSigningIds addObject:sid];
        STLogInfo("provider: first flow from sid=%{public}@ path=%{public}@", sid, path ?: @"");
    });
}

- (BOOL)proxyFlow:(NEAppProxyFlow *)flow
{
    const uint64_t flowId = [STUtils nextFlowId];
    NSString *sid = flow.metaData.sourceAppSigningIdentifier;
    NSString *path = [STUtils pathFromAuditTokenData:flow.metaData.sourceAppAuditToken];
    const char *kind = [flow isKindOfClass:[NEAppProxyTCPFlow class]]
                           ? "tcp"
                           : ([flow isKindOfClass:[NEAppProxyUDPFlow class]] ? "udp" : "other");

    [self noteFirstSightOfSigningId:sid path:path];

    NSString *reason = nil;
    const STFlowDecision decision = [_settings.policy decisionForSigningId:sid path:path reason:&reason];

    if (decision != STFlowDecisionBypass) {
        STLogDebug("flow[%{public}llu] %{public}s sid=%{public}@ -> system (%{public}@)",
                   flowId, kind, sid ?: @"", reason ?: @"");
        return NO;
    }

    nw_interface_t physical = self.physicalInterface;
    if (physical == NULL) {
        STLogError("flow[%{public}llu] %{public}s sid=%{public}@ should bypass (%{public}@) but no physical "
                   "interface is available - leaving it to the system (it will go through the tunnel)",
                   flowId, kind, sid ?: @"", reason ?: @"");
        return NO;
    }

    STLogInfo("flow[%{public}llu] %{public}s bypass sid=%{public}@ path=%{public}@ if=%{public}s (%{public}@)",
              flowId, kind, sid ?: @"", path ?: @"", nw_interface_get_name(physical) ?: "?", reason ?: @"");

    if ([flow isKindOfClass:[NEAppProxyTCPFlow class]]) {
        [FlowTCP handleFlow:(NEAppProxyTCPFlow *)flow interface:physical flowId:flowId];
        return YES;
    }
    if ([flow isKindOfClass:[NEAppProxyUDPFlow class]]) {
        [FlowUDP handleFlow:(NEAppProxyUDPFlow *)flow interface:physical flowId:flowId];
        return YES;
    }

    STLogError("flow[%{public}llu] unsupported flow class %{public}@ - leaving it to the system",
               flowId, [flow class]);
    return NO;
}

- (BOOL)handleNewFlow:(NEAppProxyFlow *)flow
{
    return [self proxyFlow:flow];
}

// Kept for macOS < 15; the system prefers the nw_endpoint_t variant below when
// it is available. Overriding a deprecated method is intentional here.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-implementations"
- (BOOL)handleNewUDPFlow:(NEAppProxyUDPFlow *)flow initialRemoteEndpoint:(NWEndpoint *)remoteEndpoint
{
    STLogDebug("provider: handleNewUDPFlow (legacy) remote=%{public}@", remoteEndpoint);
    return [self proxyFlow:flow];
}
#pragma clang diagnostic pop

- (BOOL)handleNewUDPFlow:(NEAppProxyUDPFlow *)flow initialRemoteFlowEndpoint:(nw_endpoint_t)remoteEndpoint
    API_AVAILABLE(macos(15.0))
{
    STLogDebug("provider: handleNewUDPFlow (flow endpoint) remote=%{public}s",
               remoteEndpoint != NULL ? (nw_endpoint_get_hostname(remoteEndpoint) ?: "?") : "(null)");
    return [self proxyFlow:flow];
}

@end
