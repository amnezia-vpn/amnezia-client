#ifndef iosopenvpn2ssadapter_h
#define iosopenvpn2ssadapter_h

#import <Foundation/Foundation.h>
#import "ssadapterpacketflow.h"
#import <OpenVPNAdapter/OpenVPNAdapterPacketFlow.h>

@interface ShadowSocksAdapterFlowBridge: NSObject

@property (nonatomic, weak) id<ShadowSocksAdapterPacketFlow> ssPacketFlow;
@property (nonatomic, readonly) CFSocketRef ssSocket;
@property (nonatomic, readonly) CFSocketRef packetFlowSocket;

- (BOOL)configureSocketWithError:(NSError **)error;
- (void)invalidateSocketsIfNeeded;
- (void)processPackets;

@end


#endif /* iosopenvpn2ssadapter_h */
