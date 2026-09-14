#import "Settings.h"
#import "Utils.h"

@interface STSettings ()
@property (atomic, copy, readwrite) NSString *vpnServer;
@end

@implementation STSettings

- (instancetype)init
{
    self = [super init];
    if (self) {
        _policy = [[STAppPolicy alloc] init];
        _vpnServer = @"";
    }
    return self;
}

- (BOOL)applyDictionary:(NSDictionary *)dict source:(NSString *)source
{
    NSString *where = source.length > 0 ? source : @"?";

    if (![dict isKindOfClass:[NSDictionary class]]) {
        STLogError("settings[%{public}@]: payload is not a dictionary (%{public}@)", where, [dict class]);
        return NO;
    }

    STLogInfo("settings[%{public}@]: applying keys=%{public}@", where, dict.allKeys ?: @[]);

    const STRouteMode mode = STRouteModeFromString(dict[@"mode"]);
    self.policy.mode = mode;
    STLogInfo("settings[%{public}@]: mode raw=%{public}@ parsed=%{public}s",
              where, dict[@"mode"] ?: @"(nil)", STRouteModeName(mode));
    if (mode == STRouteModeUnknown) {
        STLogError("settings[%{public}@]: unknown mode, the provider will not claim any flow", where);
    } else if (mode == STRouteModeOnly) {
        STLogError("settings[%{public}@]: include mode is not implemented, the provider will not claim any flow", where);
    }

    id server = dict[@"vpnServer"];
    if ([server isKindOfClass:[NSString class]]) {
        self.vpnServer = [server copy];
        STLogInfo("settings[%{public}@]: vpnServer=%{public}@ (ipv4=%{public}d ipv6=%{public}d)",
                  where, self.vpnServer,
                  (int)[STUtils isIPv4Address:self.vpnServer], (int)[STUtils isIPv6Address:self.vpnServer]);
        if (self.vpnServer.length == 0) {
            STLogError("settings[%{public}@]: vpnServer is empty - the VPN endpoint will not be excluded "
                       "from the proxy rules", where);
        } else if (![STUtils isIPv4Address:self.vpnServer] && ![STUtils isIPv6Address:self.vpnServer]) {
            STLogError("settings[%{public}@]: vpnServer %{public}@ is not a literal IP address - no exclude "
                       "rule will be generated for it", where, self.vpnServer);
        }
    } else if (server != nil) {
        STLogError("settings[%{public}@]: vpnServer is not a string (%{public}@)", where, [server class]);
    }

    id apps = dict[@"apps"];
    if (![apps isKindOfClass:[NSArray class]]) {
        STLogError("settings[%{public}@]: apps missing or not an array (%{public}@)", where, [apps class]);
        return NO;
    }

    STLogInfo("settings[%{public}@]: apps count=%{public}lu", where, (unsigned long)[apps count]);
    NSMutableArray<STAppEntry *> *entries = [NSMutableArray array];
    NSUInteger i = 0;
    for (id item in (NSArray *)apps) {
        if (![item isKindOfClass:[NSDictionary class]]) {
            STLogError("settings[%{public}@]: apps[%{public}lu] is not a dictionary (%{public}@)",
                       where, (unsigned long)i, [item class]);
            ++i;
            continue;
        }
        STAppEntry *entry = [[STAppEntry alloc] init];
        id bundleId = item[@"bundleId"];
        id path = item[@"path"];
        if ([bundleId isKindOfClass:[NSString class]]) {
            entry.bundleId = bundleId;
        }
        if ([path isKindOfClass:[NSString class]]) {
            entry.path = path;
        }
        STLogInfo("settings[%{public}@]: apps[%{public}lu] bundleId=%{public}@ path=%{public}@",
                  where, (unsigned long)i, entry.bundleId ?: @"", entry.path ?: @"");
        if (entry.bundleId.length > 0 || entry.path.length > 0) {
            [entries addObject:entry];
        } else {
            STLogError("settings[%{public}@]: apps[%{public}lu] has neither bundleId nor path, skipped",
                       where, (unsigned long)i);
        }
        ++i;
    }
    [self.policy replaceApps:entries];
    return YES;
}

- (NENetworkRule *)ruleForHost:(NSString *)host prefix:(NSUInteger)prefix
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    NWHostEndpoint *endpoint = [NWHostEndpoint endpointWithHostname:host port:@"0"];
    return [[NENetworkRule alloc] initWithRemoteNetwork:endpoint
                                           remotePrefix:prefix
                                           localNetwork:nil
                                            localPrefix:0
                                               protocol:NENetworkRuleProtocolAny
                                              direction:NETrafficDirectionOutbound];
#pragma clang diagnostic pop
}

- (NSArray<NENetworkRule *> *)excludedNetworkRules
{
    NSArray<NSArray *> *specs = @[
        @[ @"127.0.0.0", @8 ],
        @[ @"10.0.0.0", @8 ],
        @[ @"172.16.0.0", @12 ],
        @[ @"192.168.0.0", @16 ],
        @[ @"169.254.0.0", @16 ],
        @[ @"::1", @128 ],
        @[ @"fc00::", @7 ],
        @[ @"fe80::", @10 ],
    ];

    NSMutableArray<NENetworkRule *> *rules = [NSMutableArray array];
    for (NSArray *spec in specs) {
        NSString *host = spec[0];
        NSUInteger prefix = [spec[1] unsignedIntegerValue];
        [rules addObject:[self ruleForHost:host prefix:prefix]];
        STLogInfo("settings: exclude rule %{public}@/%{public}lu", host, (unsigned long)prefix);
    }

    NSString *server = self.vpnServer;
    if ([STUtils isIPv4Address:server]) {
        [rules addObject:[self ruleForHost:server prefix:32]];
        STLogInfo("settings: exclude rule %{public}@/32 (vpn server, ipv4)", server);
    } else if ([STUtils isIPv6Address:server]) {
        [rules addObject:[self ruleForHost:server prefix:128]];
        STLogInfo("settings: exclude rule %{public}@/128 (vpn server, ipv6)", server);
    } else {
        STLogError("settings: no exclude rule for the VPN server (value=%{public}@)", server ?: @"");
    }

    STLogInfo("settings: %{public}lu exclude rules total", (unsigned long)rules.count);
    return rules;
}

@end
