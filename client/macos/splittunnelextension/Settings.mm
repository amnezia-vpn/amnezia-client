#import "Settings.h"
#import "Utils.h"

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

- (void)applyDictionary:(NSDictionary *)dict
{
    os_log(STUtils.log, "applyDictionary keys=%{public}@", dict.allKeys ?: @[]);
    if (![dict isKindOfClass:[NSDictionary class]]) {
        os_log_error(STUtils.log, "applyDictionary: not a dict (%{public}@)", [dict class]);
        return;
    }

    NSString *mode = dict[@"mode"];
    os_log(STUtils.log, "applyDictionary mode=%{public}@", mode ?: @"(nil)");

    NSString *server = dict[@"vpnServer"];
    if ([server isKindOfClass:[NSString class]]) {
        _vpnServer = [server copy];
        os_log(STUtils.log, "applyDictionary vpnServer=%{public}@", _vpnServer);
    }

    NSArray *apps = dict[@"apps"];
    if (![apps isKindOfClass:[NSArray class]]) {
        os_log_error(STUtils.log, "applyDictionary: apps missing or not array (%{public}@)", [apps class]);
        return;
    }

    os_log(STUtils.log, "applyDictionary apps count=%{public}lu", (unsigned long)apps.count);
    NSMutableArray<STAppEntry *> *entries = [NSMutableArray array];
    NSUInteger i = 0;
    for (id item in apps) {
        if (![item isKindOfClass:[NSDictionary class]]) {
            os_log_error(STUtils.log, "applyDictionary apps[%{public}lu] not dict", (unsigned long)i);
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
        os_log(STUtils.log, "applyDictionary apps[%{public}lu] bundleId=%{public}@ path=%{public}@",
               (unsigned long)i, entry.bundleId ?: @"", entry.path ?: @"");
        if (entry.bundleId.length > 0 || entry.path.length > 0) {
            [entries addObject:entry];
        }
        ++i;
    }
    [self.policy replaceApps:entries];
}

- (NENetworkRule *)ruleForHost:(NSString *)host prefix:(NSUInteger)prefix
{
    NWHostEndpoint *endpoint = [NWHostEndpoint endpointWithHostname:host port:@"0"];
    return [[NENetworkRule alloc] initWithRemoteNetwork:endpoint
                                           remotePrefix:prefix
                                           localNetwork:nil
                                            localPrefix:0
                                               protocol:NENetworkRuleProtocolAny
                                              direction:NETrafficDirectionOutbound];
}

- (NSArray<NENetworkRule *> *)excludedNetworkRules
{
    NSMutableArray<NENetworkRule *> *rules = [NSMutableArray arrayWithArray:@[
        [self ruleForHost:@"127.0.0.0" prefix:8],
        [self ruleForHost:@"10.0.0.0" prefix:8],
        [self ruleForHost:@"172.16.0.0" prefix:12],
        [self ruleForHost:@"192.168.0.0" prefix:16],
        [self ruleForHost:@"169.254.0.0" prefix:16],
        [self ruleForHost:@"::1" prefix:128],
        [self ruleForHost:@"fc00::" prefix:7],
        [self ruleForHost:@"fe80::" prefix:10],
    ]];

    if ([STUtils isIPv4Address:self.vpnServer]) {
        [rules addObject:[self ruleForHost:self.vpnServer prefix:32]];
    }

    return rules;
}

@end
