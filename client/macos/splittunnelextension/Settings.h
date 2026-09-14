#ifndef AMNEZIA_SPLIT_TUNNEL_SETTINGS_H
#define AMNEZIA_SPLIT_TUNNEL_SETTINGS_H

#import "AppPolicy.h"

#import <Foundation/Foundation.h>
#import <NetworkExtension/NetworkExtension.h>

/*!
 * Configuration pushed from the host app, either through the start options or
 * through a provider message.
 *
 * Thread safety: `vpnServer` is an atomic copy property and `policy` publishes
 * immutable snapshots, so readers on flow queues never see a torn state.
 */
@interface STSettings : NSObject

@property (nonatomic, readonly) STAppPolicy *policy;
@property (atomic, copy, readonly) NSString *vpnServer;

/*! Returns YES when the dictionary was understood and applied. */
- (BOOL)applyDictionary:(NSDictionary *)dict source:(NSString *)source;
- (NSArray<NENetworkRule *> *)excludedNetworkRules;

@end

#endif
