#import <sspacket.h>

#include <arpa/inet.h>

@interface SSPacket () {
    NSData *_data;
    NSNumber *_protocolFamily;
}
@end

@implementation SSPacket

- (instancetype)initWithSSData:(NSData *)data {
    if (self = [super init]) {
//        NSUInteger prefix_size = sizeof(uint32_t);
//        uint32_t protocol = PF_UNSPEC;
//        [data getBytes:&protocol length:prefix_size];
//        protocol = CFSwapInt32HostToBig(protocol);
//
//        NSRange range = NSMakeRange(prefix_size, data.length - prefix_size);
//        NSData *packetData = [data subdataWithRange:range];
        
//        _data = packetData;
//        _protocolFamily = @(protocol);
        _data = data;
        _protocolFamily = @(PF_INET);
    }
    return self;
}

- (instancetype)initWithPacketFlowData:(NSData *)data protocolFamily:(NSNumber *)protocolFamily {
    if (self = [super init]) {
        _data = data;
        _protocolFamily = protocolFamily;
    }
    return self;
}

- (NSData *)vpnData {
//    uint32_t prefix = CFSwapInt32HostToBig(_protocolFamily.unsignedIntegerValue);
//    NSUInteger prefix_size = sizeof(uint32_t);
//    NSMutableData *data = [NSMutableData dataWithCapacity:prefix_size + _data.length];
//
//    [data appendBytes:&prefix length:prefix_size];
//    [data appendData:_data];
    return _data;
}

- (NSData *)ssPacketFlowData {
    return _data;
}

- (NSNumber *)protocolFamily {
    return _protocolFamily;
}

@end
