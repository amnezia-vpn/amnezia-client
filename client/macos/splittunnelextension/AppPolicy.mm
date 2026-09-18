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
        _selfBundleIdPrefix = @"";
        _selfAppPath = @"";
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

- (BOOL)isSelfExclusionUsable
{
    return self.selfBundleIdPrefix.length > 0 || self.selfAppPath.length > 0;
}

/*! Our own app, this extension, and every helper shipped inside the app bundle
 *  (AmneziaVPN-service, tun2socks, amneziawg-go, openvpn). Their traffic must
 *  never be claimed, or "only" mode would relay the tunnel through itself. */
- (BOOL)isSelfSigningId:(NSString *)signingId path:(NSString *)path
{
    NSString *prefix = self.selfBundleIdPrefix;
    if (prefix.length > 0 && signingId.length > 0) {
        if ([signingId isEqualToString:prefix]
            || [signingId hasPrefix:[prefix stringByAppendingString:@"."]]) {
            return YES;
        }
    }

    NSString *appPath = self.selfAppPath;
    if (appPath.length > 0 && path.length > 0) {
        NSString *dir = [appPath hasSuffix:@"/"] ? appPath : [appPath stringByAppendingString:@"/"];
        if ([path isEqualToString:appPath] || [path hasPrefix:dir]) {
            return YES;
        }
    }
    return NO;
}

- (BOOL)matchesList:(NSString *)signingId
               path:(NSString *)path
              index:(NSUInteger *)outIndex
             byPath:(BOOL *)outByPath
{
    NSArray<STAppEntry *> *snapshot = self.apps;
    NSUInteger index = 0;
    for (STAppEntry *app in snapshot) {
        if (signingId.length > 0 && app.bundleId.length > 0) {
            if ([signingId isEqualToString:app.bundleId]
                || [signingId hasPrefix:[app.bundleId stringByAppendingString:@"."]]) {
                if (outIndex != NULL) { *outIndex = index; }
                if (outByPath != NULL) { *outByPath = NO; }
                return YES;
            }
        }
        if (path.length > 0 && app.path.length > 0) {
            NSString *prefix = [app.path hasSuffix:@"/"] ? app.path : [app.path stringByAppendingString:@"/"];
            if ([path isEqualToString:app.path] || [path hasPrefix:prefix]) {
                if (outIndex != NULL) { *outIndex = index; }
                if (outByPath != NULL) { *outByPath = YES; }
                return YES;
            }
        }
        ++index;
    }
    return NO;
}

- (BOOL)needsPathForSigningId:(NSString *)signingId
{
    if (signingId.length == 0) {
        return YES;
    }
    // Our own family is recognised by the identifier prefix alone.
    NSString *prefix = self.selfBundleIdPrefix;
    if (prefix.length > 0
        && ([signingId isEqualToString:prefix] || [signingId hasPrefix:[prefix stringByAppendingString:@"."]])) {
        return NO;
    }
    // A list entry that carries a bundle id can match without the path; entries
    // that only carry a path cannot.
    for (STAppEntry *app in self.apps) {
        if (app.bundleId.length > 0
            && ([signingId isEqualToString:app.bundleId]
                || [signingId hasPrefix:[app.bundleId stringByAppendingString:@"."]])) {
            return NO;
        }
        if (app.path.length > 0) {
            return YES;
        }
    }
    return NO;
}

- (STFlowVerdict)verdictForSigningId:(NSString *)signingId path:(NSString *)path
{
    STFlowVerdict verdict = { STFlowDecisionSystem, STMatchKindModeUnknown, 0, NO, 0 };
    const STRouteMode mode = self.mode;
    verdict.listCount = self.apps.count;

    if (mode == STRouteModeUnknown) {
        return verdict;
    }

    // Never touch our own traffic. In "except" mode this is merely tidy; in
    // "only" mode it is what stops the provider from relaying itself.
    if ([self isSelfSigningId:signingId path:path]) {
        verdict.kind = STMatchKindSelf;
        return verdict;
    }

    // "only" mode claims everything that is not listed, so without a usable
    // self-exclusion it would capture its own sockets. Refuse instead.
    if (mode == STRouteModeOnly && ![self isSelfExclusionUsable]) {
        verdict.kind = STMatchKindUnsafeOnlyMode;
        return verdict;
    }

    NSUInteger index = 0;
    BOOL byPath = NO;
    const BOOL matched = [self matchesList:signingId path:path index:&index byPath:&byPath];
    verdict.kind = matched ? STMatchKindListed : STMatchKindNotListed;
    verdict.index = index;
    verdict.matchedByPath = byPath;

    if (mode == STRouteModeExcept) {
        verdict.decision = matched ? STFlowDecisionBypass : STFlowDecisionSystem;
    } else {
        verdict.decision = matched ? STFlowDecisionSystem : STFlowDecisionBypass;
    }
    return verdict;
}

@end

NSString *STDescribeVerdict(STFlowVerdict verdict)
{
    switch (verdict.kind) {
    case STMatchKindModeUnknown:
        return @"mode is unknown";
    case STMatchKindSelf:
        return @"own process, never proxied";
    case STMatchKindUnsafeOnlyMode:
        return @"only mode without self-exclusion data, refusing to claim anything";
    case STMatchKindListed:
        return [NSString stringWithFormat:@"listed [%lu] matched by %s",
                                          (unsigned long)verdict.index,
                                          verdict.matchedByPath ? "path" : "bundleId"];
    case STMatchKindNotListed:
    default:
        return [NSString stringWithFormat:@"not listed among %lu entries", (unsigned long)verdict.listCount];
    }
}
