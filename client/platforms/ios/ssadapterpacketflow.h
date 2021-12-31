#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@protocol ShadowSocksAdapterPacketFlow <NSObject>

- (void)readPacketsWithCompletionHandler:(void (^)(NSArray<NSData *> *packets, NSArray<NSNumber *> *protocols))completionHandler;
- (BOOL)writePackets:(NSArray<NSData *> *)packets withProtocols:(NSArray<NSNumber *> *)protocols;

@end

NS_ASSUME_NONNULL_END
