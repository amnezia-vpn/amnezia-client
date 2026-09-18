#ifndef AMNEZIA_SPLIT_TUNNEL_FLOW_UDP_H
#define AMNEZIA_SPLIT_TUNNEL_FLOW_UDP_H

#import <Network/Network.h>
#import <NetworkExtension/NetworkExtension.h>

@interface FlowUDP : NSObject

/*! Takes ownership of `flow` and proxies its datagrams out of `interface`.
 *  `flowId` only correlates log lines. */
+ (void)handleFlow:(NEAppProxyUDPFlow *)flow interface:(nw_interface_t)interface flowId:(uint64_t)flowId;

/*! Tears down every live session. Called when the proxy stops, so the
 *  registry does not keep finished sessions alive across restarts. */
+ (void)closeAll;

@end

#endif
