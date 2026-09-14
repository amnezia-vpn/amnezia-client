#ifndef AMNEZIA_SPLIT_TUNNEL_APP_POLICY_H
#define AMNEZIA_SPLIT_TUNNEL_APP_POLICY_H

#import <Foundation/Foundation.h>

@interface STAppEntry : NSObject
@property (nonatomic, copy) NSString *bundleId;
@property (nonatomic, copy) NSString *path;
@end

@interface STAppPolicy : NSObject

- (void)replaceApps:(NSArray<STAppEntry *> *)apps;
- (BOOL)shouldExcludeSigningId:(NSString *)signingId path:(NSString *)path;

@end

#endif
