#ifndef AMNEZIA_SPLIT_TUNNEL_UTILS_H
#define AMNEZIA_SPLIT_TUNNEL_UTILS_H

#import <Foundation/Foundation.h>
#import <Network/Network.h>
#import <dispatch/dispatch.h>
#import <os/log.h>

/*!
 * Logging levels used across the extension:
 *   STLogInfo  - lifecycle events (start/stop, settings, per-flow open/close).
 *                Always recorded.
 *   STLogDebug - per-read/per-datagram chatter. Recorded only when debug logging
 *                is enabled for this subsystem:
 *                  sudo log config --mode "level:debug" --subsystem org.amnezia.AmneziaVPN.network-extension
 *                Stream with:
 *                  log stream --predicate 'subsystem == "org.amnezia.AmneziaVPN.network-extension"' --level debug
 */
#define STLogInfo(fmt, ...)  os_log(STUtils.log, fmt, ##__VA_ARGS__)
#define STLogDebug(fmt, ...) os_log_debug(STUtils.log, fmt, ##__VA_ARGS__)
#define STLogError(fmt, ...) os_log_error(STUtils.log, fmt, ##__VA_ARGS__)

@interface STUtils : NSObject

+ (os_log_t)log;

/*! Serial queue guarding every shared mutable container in the extension
 *  (flow registries, the "seen signing ids" set, per-flow UDP connection maps).
 *  Flow callbacks and Network.framework handlers run on different queues, so
 *  every mutation of a shared NSMutable* must happen here. */
+ (dispatch_queue_t)stateQueue;

+ (NSString *)pathFromAuditTokenData:(NSData *)tokenData;
+ (nw_endpoint_t)copyEndpointFromHost:(NSString *)host port:(NSString *)port;
+ (BOOL)isIPv4Address:(NSString *)value;
+ (BOOL)isIPv6Address:(NSString *)value;
+ (NSData *)dataFromDispatchData:(dispatch_data_t)content;

/*! Human readable nw_connection_state_t, for logs. */
+ (const char *)connectionStateName:(nw_connection_state_t)state;
/*! Turns an nw_error_t into an NSError for uniform logging. Returns nil for NULL. */
+ (NSError *)errorFromNWError:(nw_error_t)error;
/*! Monotonically increasing id used to correlate log lines of one flow. */
+ (uint64_t)nextFlowId;

@end

#endif
