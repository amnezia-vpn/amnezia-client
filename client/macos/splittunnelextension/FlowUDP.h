#ifndef AMNEZIA_SPLIT_TUNNEL_FLOW_UDP_H
#define AMNEZIA_SPLIT_TUNNEL_FLOW_UDP_H

#import <Network/Network.h>
#import <NetworkExtension/NetworkExtension.h>

@interface FlowUDP : NSObject

+ (void)handleFlow:(NEAppProxyUDPFlow *)flow interface:(nw_interface_t)interface;

@end

#endif
