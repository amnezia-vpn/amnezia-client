#import "TransparentProxyProvider.h"

#import "FlowTCP.h"
#import "FlowUDP.h"
#import "Settings.h"
#import "Utils.h"

#import <Network/Network.h>
#import <os/log.h>

@implementation TransparentProxyProvider {
    STSettings *_settings;
    nw_path_monitor_t _pathMonitor;
    nw_interface_t _physicalInterface;
    NSMutableSet<NSString *> *_seenSigningIds;
}

- (instancetype)init
{
    self = [super init];
    if (self) {
        _settings = [[STSettings alloc] init];
        _seenSigningIds = [NSMutableSet set];
    }
    return self;
}

- (void)startPathMonitor
{
    if (_pathMonitor != NULL) {
        return;
    }
    _pathMonitor = nw_path_monitor_create();
    nw_path_monitor_set_queue(_pathMonitor, dispatch_get_main_queue());

    __weak TransparentProxyProvider *weakSelf = self;
    nw_path_monitor_set_update_handler(_pathMonitor, ^(nw_path_t path) {
        TransparentProxyProvider *strongSelf = weakSelf;
        if (strongSelf == nil) {
            return;
        }
        __block nw_interface_t found = NULL;
        nw_path_enumerate_interfaces(path, ^bool(nw_interface_t interface) {
            nw_interface_type_t type = nw_interface_get_type(interface);
        if (type == nw_interface_type_wifi || type == nw_interface_type_wired) {
                found = interface;
                os_log(STUtils.log, "path monitor physical if=%{public}s type=%d",
                       nw_interface_get_name(interface) ?: "?", (int)type);
                return false;
            }
            return true;
        });
        strongSelf->_physicalInterface = found;
    });
    nw_path_monitor_start(_pathMonitor);
}

- (void)stopPathMonitor
{
    if (_pathMonitor != NULL) {
        nw_path_monitor_cancel(_pathMonitor);
        _pathMonitor = NULL;
    }
    _physicalInterface = NULL;
}

- (void)startProxyWithOptions:(NSDictionary *)options completionHandler:(void (^)(NSError *))completionHandler
{
    os_log(STUtils.log, "startProxy options=%{public}@", options ?: @{});
    [_settings applyDictionary:options];
    [self startPathMonitor];

    NETransparentProxyNetworkSettings *settings =
        [[NETransparentProxyNetworkSettings alloc] initWithTunnelRemoteAddress:@"127.0.0.1"];

    // Wildcard include: both endpoints nil. An explicit ::/0 NWHostEndpoint drops the whole ruleset.
    NENetworkRule *allOutbound = [[NENetworkRule alloc] initWithRemoteNetwork:nil
                                                                 remotePrefix:0
                                                                 localNetwork:nil
                                                                  localPrefix:0
                                                                     protocol:NENetworkRuleProtocolAny
                                                                    direction:NETrafficDirectionOutbound];
    settings.includedNetworkRules = @[ allOutbound ];
    settings.excludedNetworkRules = [_settings excludedNetworkRules];

    [self setTunnelNetworkSettings:settings completionHandler:^(NSError *error) {
        if (error != nil) {
            os_log_error(STUtils.log, "setTunnelNetworkSettings failed: %{public}@", error);
        } else {
            os_log(STUtils.log, "setTunnelNetworkSettings ok included=1 excluded=%{public}lu",
                   (unsigned long)settings.excludedNetworkRules.count);
        }
        completionHandler(error);
    }];
}

- (void)stopProxyWithReason:(NEProviderStopReason)reason completionHandler:(void (^)(void))completionHandler
{
    (void)reason;
    os_log(STUtils.log, "stopProxy");
    [self stopPathMonitor];
    completionHandler();
}

- (void)handleAppMessage:(NSData *)messageData completionHandler:(void (^)(NSData *))completionHandler
{
    NSError *error = nil;
    id json = [NSJSONSerialization JSONObjectWithData:messageData options:0 error:&error];
    if (error == nil && [json isKindOfClass:[NSDictionary class]]) {
        os_log(STUtils.log, "handleAppMessage %{public}@", json);
        [_settings applyDictionary:json];
        os_log(STUtils.log, "updated app policy from provider message");
    } else {
        os_log_error(STUtils.log, "handleAppMessage parse failed: %{public}@", error);
    }
    if (completionHandler != nil) {
        completionHandler(nil);
    }
}

- (BOOL)proxyFlow:(NEAppProxyFlow *)flow
{
    NSString *sid = flow.metaData.sourceAppSigningIdentifier;
    NSString *path = [STUtils pathFromAuditTokenData:flow.metaData.sourceAppAuditToken];
    const BOOL firstSeen = (sid.length > 0 && ![_seenSigningIds containsObject:sid]);
    if (firstSeen) {
        [_seenSigningIds addObject:sid];
        os_log(STUtils.log, "flow first sid=%{public}@ path=%{public}@", sid, path ?: @"");
    }
    if (![_settings.policy shouldExcludeSigningId:sid path:path]) {
        return NO;
    }
    if (_physicalInterface == NULL) {
        os_log_error(STUtils.log, "exclude hit but no physical interface sid=%{public}@", sid);
        return NO;
    }

    os_log(STUtils.log, "exclude %{public}@ path %{public}@ if=%{public}s",
          sid, path, nw_interface_get_name(_physicalInterface) ?: "?");

    if ([flow isKindOfClass:[NEAppProxyTCPFlow class]]) {
        [FlowTCP handleFlow:(NEAppProxyTCPFlow *)flow interface:_physicalInterface];
        return YES;
    }
    if ([flow isKindOfClass:[NEAppProxyUDPFlow class]]) {
        [FlowUDP handleFlow:(NEAppProxyUDPFlow *)flow interface:_physicalInterface];
        return YES;
    }
    return NO;
}

- (BOOL)handleNewFlow:(NEAppProxyFlow *)flow
{
    return [self proxyFlow:flow];
}

- (BOOL)handleNewUDPFlow:(NEAppProxyUDPFlow *)flow initialRemoteEndpoint:(NWEndpoint *)remoteEndpoint
{
    (void)remoteEndpoint;
    return [self proxyFlow:flow];
}

@end
