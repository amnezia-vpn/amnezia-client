#import <Foundation/Foundation.h>
#import <NetworkExtension/NetworkExtension.h>

#import "TransparentProxyProvider.h"

#import <grp.h>
#import <sys/types.h>
#import <unistd.h>

static void JoinAmnvpnGroup(void)
{
    struct group *grp = getgrnam("amnvpn");
    if (grp == NULL) {
        return;
    }
    if (setgid(grp->gr_gid) != 0) {
        return;
    }
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    @autoreleasepool {
        JoinAmnvpnGroup();
        [TransparentProxyProvider class];
        [NEProvider startSystemExtensionMode];
        dispatch_main();
    }
    return 0;
}
