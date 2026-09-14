#ifndef AMNEZIA_SPLIT_TUNNEL_SETTINGS_H
#define AMNEZIA_SPLIT_TUNNEL_SETTINGS_H

#import "AppPolicy.h"

#import <Foundation/Foundation.h>
#import <NetworkExtension/NetworkExtension.h>

@interface STSettings : NSObject

@property (nonatomic, readonly) STAppPolicy *policy;
@property (nonatomic, copy, readonly) NSString *vpnServer;

- (void)applyDictionary:(NSDictionary *)dict;
- (NSArray<NENetworkRule *> *)excludedNetworkRules;

@end

#endif
