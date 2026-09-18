#ifndef AMNEZIA_SPLIT_TUNNEL_UTILS_H
#define AMNEZIA_SPLIT_TUNNEL_UTILS_H

#import <Foundation/Foundation.h>
#import <Network/Network.h>
#import <dispatch/dispatch.h>
#import <os/log.h>

typedef NS_ENUM(NSInteger, STLogLevel) {
    STLogLevelDebug = 0,
    STLogLevelInfo,
    STLogLevelError,
};

/*!
 * Every log line goes to two places:
 *
 *   - os_log, under the extension's bundle identifier as the subsystem, so
 *     `log stream --predicate 'subsystem == "..."'` keeps working;
 *   - a plain text file next to the app and service logs, so the whole picture
 *     can be collected from one folder.
 *
 * The file is only opened once the host app tells the extension where to write:
 * a system extension runs as root, so it cannot derive the user's Documents
 * folder on its own. The directory arrives in the start options as "logDir".
 *
 * Format strings use os_log syntax (%{public}@ and friends). For the file the
 * annotations are stripped and the line is rendered with NSString formatting.
 * Nothing is rendered at all when neither sink would accept the level, so the
 * per-flow debug lines stay cheap while debug logging is off.
 */
FOUNDATION_EXPORT void STLogWrite(STLogLevel level, const char *fmt, ...);

#define STLogInfo(fmt, ...)  STLogWrite(STLogLevelInfo, fmt, ##__VA_ARGS__)
#define STLogDebug(fmt, ...) STLogWrite(STLogLevelDebug, fmt, ##__VA_ARGS__)
#define STLogError(fmt, ...) STLogWrite(STLogLevelError, fmt, ##__VA_ARGS__)

@interface STFileLog : NSObject

/*! Opens <directory>/AmneziaVPNSplitTunnel_root/AmneziaVPNSplitTunnel.log.
 *  Safe to call repeatedly with the same directory - it is a no-op then. */
+ (void)configureWithDirectory:(NSString *)directory;
/*! Whether this level would be written to the file. */
+ (BOOL)acceptsLevel:(STLogLevel)level;
/*! Turns the per-read/per-datagram chatter on. Off by default. */
+ (void)setDebugEnabled:(BOOL)enabled;
+ (BOOL)isDebugEnabled;
+ (NSString *)currentPath;

@end

@interface STUtils : NSObject

+ (os_log_t)log;

/*! Serial queue guarding every shared mutable container in the extension
 *  (flow registries, the "seen signing ids" set). */
+ (dispatch_queue_t)stateQueue;

+ (NSString *)pathFromAuditTokenData:(NSData *)tokenData;
+ (nw_endpoint_t)copyEndpointFromHost:(NSString *)host port:(NSString *)port;
+ (BOOL)isIPv4Address:(NSString *)value;
+ (BOOL)isIPv6Address:(NSString *)value;
+ (NSData *)dataFromDispatchData:(dispatch_data_t)content;

+ (const char *)connectionStateName:(nw_connection_state_t)state;
/*! Why the system says a connection cannot proceed: path status, the reason it
 *  is unsatisfied, and the interfaces the path does offer. A connection pinned
 *  to a physical interface sits in "waiting" with a nil error when NECP has no
 *  usable path for it, and the error alone never says which of those it is. */
+ (NSString *)describeConnectionPath:(nw_connection_t)connection;
+ (NSError *)errorFromNWError:(nw_error_t)error;
+ (uint64_t)nextFlowId;

@end

/*! Cumulative relay counters, reported periodically by the provider. */
@interface STStats : NSObject
+ (void)tcpOpened;
+ (void)tcpClosedWithOut:(uint64_t)out in:(uint64_t)in;
+ (void)udpOpened;
+ (void)udpClosedWithOut:(uint64_t)out in:(uint64_t)in;
+ (void)flowSentToTunnel;
/*! One line summarising everything seen so far. */
+ (NSString *)summary;
@end

#endif
