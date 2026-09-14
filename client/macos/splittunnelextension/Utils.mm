#import "Utils.h"

#import <Security/Security.h>
#import <arpa/inet.h>
#import <os/log.h>
#import <string.h>

@implementation STUtils

+ (os_log_t)log
{
    static os_log_t logger;
    static dispatch_once_t once;
    dispatch_once(&once, ^{
#ifdef CLIENT_MACOS_ST_BUNDLE_ID
        logger = os_log_create(CLIENT_MACOS_ST_BUNDLE_ID, "proxy");
#else
        logger = os_log_create("org.amnezia.AmneziaVPN.split-tunnel", "proxy");
#endif
    });
    return logger;
}

+ (NSData *)dataFromDispatchData:(dispatch_data_t)content
{
    if (content == NULL) {
        return nil;
    }
    NSMutableData *data = [NSMutableData data];
    dispatch_data_apply(content, ^bool(dispatch_data_t region, size_t offset, const void *buffer, size_t size) {
        (void)region;
        (void)offset;
        [data appendBytes:buffer length:size];
        return true;
    });
    return data;
}

+ (NSString *)pathFromAuditTokenData:(NSData *)tokenData
{
    if (tokenData.length != sizeof(audit_token_t)) {
        return nil;
    }

    NSDictionary *attributes = @{ (__bridge NSString *)kSecGuestAttributeAudit : tokenData };
    SecCodeRef code = NULL;
    OSStatus status = SecCodeCopyGuestWithAttributes(NULL, (__bridge CFDictionaryRef)attributes, kSecCSDefaultFlags, &code);
    if (status != errSecSuccess || code == NULL) {
        os_log_error(STUtils.log, "pathFromAuditToken SecCodeCopyGuest status=%d", (int)status);
        return nil;
    }

    CFURLRef url = NULL;
    status = SecCodeCopyPath(code, kSecCSDefaultFlags, &url);
    CFRelease(code);
    if (status != errSecSuccess || url == NULL) {
        return nil;
    }

    NSString *path = [(__bridge NSURL *)url path];
    CFRelease(url);
    return path;
}

+ (nw_endpoint_t)copyEndpointFromHost:(NSString *)host port:(NSString *)port
{
    if (host.length == 0 || port.length == 0) {
        return NULL;
    }
    return nw_endpoint_create_host(host.UTF8String, port.UTF8String);
}

+ (BOOL)isIPv4Address:(NSString *)value
{
    if (value.length == 0) {
        return NO;
    }
    struct in_addr addr;
    return inet_pton(AF_INET, value.UTF8String, &addr) == 1;
}

@end
