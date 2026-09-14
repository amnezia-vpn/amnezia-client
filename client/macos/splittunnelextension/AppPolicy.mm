#import "AppPolicy.h"
#import "Utils.h"

@implementation STAppEntry
@end

@implementation STAppPolicy {
    NSArray<STAppEntry *> *_apps;
}

- (instancetype)init
{
    self = [super init];
    if (self) {
        _apps = @[];
    }
    return self;
}

- (void)replaceApps:(NSArray<STAppEntry *> *)apps
{
    _apps = [apps copy] ?: @[];
    os_log(STUtils.log, "policy replaceApps count=%{public}lu", (unsigned long)_apps.count);
    NSUInteger i = 0;
    for (STAppEntry *app in _apps) {
        os_log(STUtils.log, "policy[%{public}lu] bundleId=%{public}@ path=%{public}@",
               (unsigned long)i, app.bundleId ?: @"", app.path ?: @"");
        ++i;
    }
}

- (BOOL)shouldExcludeSigningId:(NSString *)signingId path:(NSString *)path
{
    for (STAppEntry *app in _apps) {
        if (signingId.length > 0 && app.bundleId.length > 0) {
            if ([signingId isEqualToString:app.bundleId]
                || [signingId hasPrefix:[app.bundleId stringByAppendingString:@"."]]) {
                os_log(STUtils.log, "policy MATCH sid %{public}@ ~= bundleId %{public}@", signingId, app.bundleId);
                return YES;
            }
        }
        if (path.length > 0 && app.path.length > 0) {
            NSString *prefix = app.path;
            if (![prefix hasSuffix:@"/"]) {
                prefix = [prefix stringByAppendingString:@"/"];
            }
            if ([path isEqualToString:app.path] || [path hasPrefix:prefix]) {
                os_log(STUtils.log, "policy MATCH path %{public}@ ~= %{public}@", path, app.path);
                return YES;
            }
        }
    }
    return NO;
}

@end
