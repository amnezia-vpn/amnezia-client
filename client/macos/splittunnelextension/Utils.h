#ifndef AMNEZIA_SPLIT_TUNNEL_UTILS_H
#define AMNEZIA_SPLIT_TUNNEL_UTILS_H

#import <Foundation/Foundation.h>
#import <Network/Network.h>
#import <dispatch/dispatch.h>
#import <os/log.h>

@interface STUtils : NSObject

+ (os_log_t)log;
+ (NSString *)pathFromAuditTokenData:(NSData *)tokenData;
+ (nw_endpoint_t)copyEndpointFromHost:(NSString *)host port:(NSString *)port;
+ (BOOL)isIPv4Address:(NSString *)value;
+ (NSData *)dataFromDispatchData:(dispatch_data_t)content;

@end

#endif
