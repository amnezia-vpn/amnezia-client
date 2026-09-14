#import <Foundation/Foundation.h>
#import <NetworkExtension/NetworkExtension.h>

#import "TransparentProxyProvider.h"
#import "Utils.h"

#import <errno.h>
#import <grp.h>
#import <string.h>
#import <sys/types.h>
#import <unistd.h>

/*!
 * The extension runs as root. It also joins the "amnvpn" group so that the pf
 * anchor amn.320.allowExcludedApps ("pass out ... group amnvpn") lets the
 * bypassed traffic through while the kill switch is armed.
 */
static void JoinAmnvpnGroup(void)
{
    struct group *grp = getgrnam("amnvpn");
    if (grp == NULL) {
        STLogError("main: group 'amnvpn' not found (errno=%{public}d %{public}s) - bypass traffic will be "
                   "blocked whenever the kill switch is armed",
                   errno, strerror(errno));
        return;
    }
    if (setgid(grp->gr_gid) != 0) {
        STLogError("main: setgid(%{public}d) failed: errno=%{public}d %{public}s",
                   (int)grp->gr_gid, errno, strerror(errno));
        return;
    }
    STLogInfo("main: joined group amnvpn gid=%{public}d", (int)grp->gr_gid);
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    @autoreleasepool {
        NSBundle *bundle = [NSBundle mainBundle];
        STLogInfo("main: starting %{public}@ version=%{public}@ (%{public}@) pid=%{public}d uid=%{public}d",
                  bundle.bundleIdentifier ?: @"?",
                  bundle.infoDictionary[@"CFBundleShortVersionString"] ?: @"?",
                  bundle.infoDictionary[@"CFBundleVersion"] ?: @"?",
                  (int)getpid(), (int)getuid());

        JoinAmnvpnGroup();

        [TransparentProxyProvider class];
        STLogInfo("main: entering system extension mode");
        [NEProvider startSystemExtensionMode];
        dispatch_main();
    }
    return 0;
}
