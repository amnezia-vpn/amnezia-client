#import "iosopenvpn2ssadapter.h"

#include <sys/socket.h>
#include <arpa/inet.h>

#import "sspacket.h"
#import "ssadapterpacketflow.h"

@implementation ShadowSocksAdapterFlowBridge

# pragma mark - Sockets Configuration

static void SocketCallback(CFSocketRef socket, CFSocketCallBackType type, CFDataRef address, const void *data, void *obj) {
    if (type != kCFSocketDataCallBack) {
        return;
    }
    SSPacket *packet = [[SSPacket alloc] initWithSSData:data];
    ShadowSocksAdapterFlowBridge *bridge = (__bridge ShadowSocksAdapterFlowBridge*)obj;
    [bridge writePackets:@[packet] toPacketFlow:bridge.ssPacketFlow];
}

- (BOOL)configureSocketWithError:(NSError * __autoreleasing *)error {
    int sockets[2];
    if (socketpair(PF_LOCAL, SOCK_DGRAM, IPPROTO_IP, sockets) == -1) {
        if (error) {
            NSDictionary *userInfo = @{
                // TODO: handle
            };
            *error = [NSError errorWithDomain:@"Some domain" code:100 userInfo:userInfo];
        }
        return NO;
    }
    CFSocketContext socketCtx = {0, (__bridge void *)self, NULL, NULL, NULL};
    _packetFlowSocket = CFSocketCreateWithNative(kCFAllocatorDefault, sockets[0], kCFSocketDataCallBack, SocketCallback, &socketCtx);
    _ssSocket = CFSocketCreateWithNative(kCFAllocatorDefault, sockets[1], kCFSocketNoCallBack, NULL, NULL);
    if (!(_packetFlowSocket && _ssSocket)) {
        if (error) {
            NSDictionary *userInfo = @{
                // TODO: handle
            };
            *error = [NSError errorWithDomain:@"Some domain" code:100 userInfo:userInfo];
        }
        return NO;
    }
    if (!([self configureOptionsForSocket:_packetFlowSocket error:error] && [self configureOptionsForSocket:_ssSocket error:error])) {
        return NO;
    }
    CFRunLoopSourceRef packetFlowSocketSource = CFSocketCreateRunLoopSource(kCFAllocatorDefault, _packetFlowSocket, 0);
    CFRunLoopAddSource(CFRunLoopGetMain(), packetFlowSocketSource, kCFRunLoopDefaultMode);
    CFRelease(packetFlowSocketSource);
    return YES;
}

- (void)invalidateSocketsIfNeeded {
    if (_ssSocket) {
        CFSocketInvalidate(_ssSocket);
        CFRelease(_ssSocket);
        _ssSocket = NULL;
    }
    if (_packetFlowSocket) {
        CFSocketInvalidate(_packetFlowSocket);
        CFRelease(_packetFlowSocket);
        _packetFlowSocket = NULL;
    }
}

- (void)processPackets {
    NSAssert(self.ssPacketFlow != nil, @"packetFlow property shouldn't be nil, set it before start reading packets.");
    __weak typeof(self) weakSelf = self;
    [self.ssPacketFlow readPacketsWithCompletionHandler:^(NSArray<NSData *> *packets, NSArray<NSNumber *> *protocols) {
        __strong typeof(self) self = weakSelf;
        [self writePackets:packets protocols:protocols toSocket:self.packetFlowSocket];
        [self processPackets];
    }];
}

- (void)dealloc {
    [self invalidateSocketsIfNeeded];
    [super dealloc];
}

# pragma mark - Socket configuration
- (BOOL)configureOptionsForSocket:(CFSocketRef)socket error:(NSError * __autoreleasing *)error {
    CFSocketNativeHandle socketHandle = CFSocketGetNative(socket);
    
    int buf_value = 65536;
    socklen_t buf_len = sizeof(buf_value);
    
    if (setsockopt(socketHandle, SOL_SOCKET, SO_RCVBUF, &buf_value, buf_len) == -1) {
        if (error) {
            NSDictionary *userInfo = @{
                // TODO: handle
            };
            *error = [NSError errorWithDomain:@"Some domain" code:100 userInfo:userInfo];
        }
        
        return NO;
    }
    
    if (setsockopt(socketHandle, SOL_SOCKET, SO_SNDBUF, &buf_value, buf_len) == -1) {
        if (error) {
            NSDictionary *userInfo = @{
                // TODO: handle
            };
            *error = [NSError errorWithDomain:@"Some domain" code:100 userInfo:userInfo];
        }
        
        return NO;
    }
    return YES;
}

# pragma mark - Protocol methods
- (void)writePackets:(NSArray<SSPacket *> *)packets toPacketFlow:(id<ShadowSocksAdapterPacketFlow>)packetFlow {
    NSAssert(self.ssPacketFlow != nil, @"packetFlow shouldn't be nil, check provided parameter before start writing packets.");
    
    NSMutableArray<NSData *> *flowPackets = [[NSMutableArray alloc] init];
    NSMutableArray<NSNumber *> *protocols = [[NSMutableArray alloc] init];
    
    [packets enumerateObjectsUsingBlock:^(SSPacket * _Nonnull packet, NSUInteger idx, BOOL * _Nonnull stop) {
        [flowPackets addObject:packet.ssPacketFlowData];
        [protocols addObject:packet.protocolFamily];
    }];
    
    [packetFlow writePackets:flowPackets withProtocols:protocols];
}

- (void)writePackets:(NSArray<NSData *> *)packets protocols:(NSArray<NSNumber *> *)protocols toSocket:(CFSocketRef)socket {
    if (socket == NULL) { return; }
    
    [packets enumerateObjectsUsingBlock:^(NSData *data, NSUInteger idx, BOOL *stop) {
        NSNumber *protocolFamily = protocols[idx];
        SSPacket *packet = [[SSPacket alloc] initWithPacketFlowData:data protocolFamily:protocolFamily];
        
        CFSocketSendData(socket, NULL, (CFDataRef)packet.vpnData, 0.05);
    }];
}

@end
