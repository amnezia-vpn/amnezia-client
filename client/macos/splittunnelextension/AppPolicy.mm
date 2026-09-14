#import "AppPolicy.h"
#import "Utils.h"

STRouteMode STRouteModeFromString(NSString *value)
{
    if ([value isKindOfClass:[NSString class]]) {
        if ([value isEqualToString:@"except"]) {
            return STRouteModeExcept;
        }
        if ([value isEqualToString:@"only"]) {
            return STRouteModeOnly;
        }
    }
    return STRouteModeUnknown;
}

const char *STRouteModeName(STRouteMode mode)
{
    switch (mode) {
    case STRouteModeExcept: return "except";
    case STRouteModeOnly: return "only";
    case STRouteModeUnknown:
    default: return "unknown";
    }
}

@implementation STAppEntry

- (NSString *)description
{
    return [NSString stringWithFormat:@"<STAppEntry bundleId=%@ path=%@>", self.bundleId ?: @"", self.path ?: @""];
}

@end

@interface STAppPolicy ()
@property (atomic, copy, readwrite) NSArray<STAppEntry *> *apps;
@end

@implementation STAppPolicy

- (instancetype)init
{
    self = [super init];
    if (self) {
        _apps = @[];
        _mode = STRouteModeUnknown;
    }
    return self;
}

- (void)replaceApps:(NSArray<STAppEntry *> *)apps
{
    NSArray<STAppEntry *> *snapshot = [apps copy] ?: @[];
    self.apps = snapshot;
    STLogInfo("policy: replaceApps count=%{public}lu mode=%{public}s",
              (unsigned long)snapshot.count, STRouteModeName(self.mode));
    NSUInteger i = 0;
    for (STAppEntry *app in snapshot) {
        STLogInfo("policy:   [%{public}lu] bundleId=%{public}@ path=%{public}@",
                  (unsigned long)i, app.bundleId ?: @"", app.path ?: @"");
        ++i;
    }
}

- (STFlowDecision)decisionForSigningId:(NSString *)signingId
                                  path:(NSString *)path
                                reason:(NSString *__autoreleasing *)reason
{
    const STRouteMode mode = self.mode;

    if (mode != STRouteModeExcept) {
        // "only" (include) mode needs the flow to be re-opened on the tunnel
        // interface, which this provider does not implement. Refuse explicitly
        // rather than silently behaving like "except".
        if (reason != NULL) {
            *reason = [NSString stringWithFormat:@"mode=%s not supported, leaving flow to the system",
                                                 STRouteModeName(mode)];
        }
        return STFlowDecisionSystem;
    }

    NSArray<STAppEntry *> *snapshot = self.apps;
    NSUInteger index = 0;
    for (STAppEntry *app in snapshot) {
        if (signingId.length > 0 && app.bundleId.length > 0) {
            if ([signingId isEqualToString:app.bundleId]
                || [signingId hasPrefix:[app.bundleId stringByAppendingString:@"."]]) {
                if (reason != NULL) {
                    *reason = [NSString stringWithFormat:@"bundleId match [%lu] %@ ~= %@",
                                                         (unsigned long)index, signingId, app.bundleId];
                }
                return STFlowDecisionBypass;
            }
        }
        if (path.length > 0 && app.path.length > 0) {
            NSString *prefix = app.path;
            if (![prefix hasSuffix:@"/"]) {
                prefix = [prefix stringByAppendingString:@"/"];
            }
            if ([path isEqualToString:app.path] || [path hasPrefix:prefix]) {
                if (reason != NULL) {
                    *reason = [NSString stringWithFormat:@"path match [%lu] %@ ~= %@",
                                                         (unsigned long)index, path, app.path];
                }
                return STFlowDecisionBypass;
            }
        }
        ++index;
    }

    if (reason != NULL) {
        *reason = [NSString stringWithFormat:@"no match among %lu entries", (unsigned long)snapshot.count];
    }
    return STFlowDecisionSystem;
}

@end
