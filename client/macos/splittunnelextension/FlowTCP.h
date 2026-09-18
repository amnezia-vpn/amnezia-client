#ifndef AMNEZIA_SPLIT_TUNNEL_FLOW_TCP_H
#define AMNEZIA_SPLIT_TUNNEL_FLOW_TCP_H

#import <Network/Network.h>
#import <NetworkExtension/NetworkExtension.h>

@interface FlowTCP : NSObject

/*! Takes ownership of `flow` and proxies it out of `interface`.
 *  `flowId` only correlates log lines. */
+ (void)handleFlow:(NEAppProxyTCPFlow *)flow interface:(nw_interface_t)interface flowId:(uint64_t)flowId;

/*! Tears down every live session. Called when the proxy stops, so the
 *  registry does not keep finished sessions alive across restarts. */
+ (void)closeAll;

@end

#endif
