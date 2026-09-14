#ifndef AMNEZIA_SPLIT_TUNNEL_FLOW_TCP_H
#define AMNEZIA_SPLIT_TUNNEL_FLOW_TCP_H

#import <Network/Network.h>
#import <NetworkExtension/NetworkExtension.h>

@interface FlowTCP : NSObject

+ (void)handleFlow:(NEAppProxyTCPFlow *)flow interface:(nw_interface_t)interface;

@end

#endif
