#import "Utils.h"

#import <Security/Security.h>
#import <arpa/inet.h>
#import <os/log.h>
#import <stdatomic.h>
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

+ (dispatch_queue_t)stateQueue
{
    static dispatch_queue_t queue;
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        queue = dispatch_queue_create("org.amnezia.split-tunnel.state", DISPATCH_QUEUE_SERIAL);
    });
    return queue;
}

+ (uint64_t)nextFlowId
{
    static _Atomic uint64_t counter = 0;
    return atomic_fetch_add_explicit(&counter, 1, memory_order_relaxed) + 1;
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
        STLogDebug("pathFromAuditToken: unexpected token length %{public}lu (want %{public}lu)",
                   (unsigned long)tokenData.length, (unsigned long)sizeof(audit_token_t));
        return nil;
    }

    NSDictionary *attributes = @{ (__bridge NSString *)kSecGuestAttributeAudit : tokenData };
    SecCodeRef code = NULL;
    OSStatus status = SecCodeCopyGuestWithAttributes(NULL, (__bridge CFDictionaryRef)attributes, kSecCSDefaultFlags, &code);
    if (status != errSecSuccess || code == NULL) {
        STLogError("pathFromAuditToken SecCodeCopyGuest status=%{public}d", (int)status);
        return nil;
    }

    CFURLRef url = NULL;
    status = SecCodeCopyPath(code, kSecCSDefaultFlags, &url);
    CFRelease(code);
    if (status != errSecSuccess || url == NULL) {
        STLogError("pathFromAuditToken SecCodeCopyPath status=%{public}d", (int)status);
        return nil;
    }

    NSString *path = [(__bridge NSURL *)url path];
    CFRelease(url);
    return path;
}

+ (nw_endpoint_t)copyEndpointFromHost:(NSString *)host port:(NSString *)port
{
    if (host.length == 0 || port.length == 0) {
        STLogError("copyEndpointFromHost: empty host=%{public}@ port=%{public}@", host ?: @"", port ?: @"");
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

+ (BOOL)isIPv6Address:(NSString *)value
{
    if (value.length == 0) {
        return NO;
    }
    struct in6_addr addr;
    return inet_pton(AF_INET6, value.UTF8String, &addr) == 1;
}

+ (const char *)connectionStateName:(nw_connection_state_t)state
{
    switch (state) {
    case nw_connection_state_invalid: return "invalid";
    case nw_connection_state_waiting: return "waiting";
    case nw_connection_state_preparing: return "preparing";
    case nw_connection_state_ready: return "ready";
    case nw_connection_state_failed: return "failed";
    case nw_connection_state_cancelled: return "cancelled";
    default: return "unknown";
    }
}

+ (NSError *)errorFromNWError:(nw_error_t)error
{
    if (error == NULL) {
        return nil;
    }
    return CFBridgingRelease(nw_error_copy_cf_error(error));
}

@end
