#ifndef sspacket_h
#define sspacket_h

#import <Foundation/Foundation.h>

@interface SSPacket : NSObject
@property (readonly, nonatomic) NSData *vpnData;
@property (readonly, nonatomic) NSData *ssPacketFlowData;
@property (readonly,nonatomic) NSNumber *protocolFamily;

- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithSSData:(NSData *)data NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithPacketFlowData:(NSData *)data protocolFamily:(NSNumber *)protocolFamily NS_DESIGNATED_INITIALIZER;
@end

#endif /* sspacket_h */
