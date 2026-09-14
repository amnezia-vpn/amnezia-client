#ifndef AMNEZIA_SPLIT_TUNNEL_APP_POLICY_H
#define AMNEZIA_SPLIT_TUNNEL_APP_POLICY_H

#import <Foundation/Foundation.h>

/*! Route mode sent by the host app in the "mode" key. */
typedef NS_ENUM(NSInteger, STRouteMode) {
    /*! Unknown/unset - the provider claims nothing and lets the system route. */
    STRouteModeUnknown = 0,
    /*! "except": listed apps bypass the tunnel, everything else stays in it. */
    STRouteModeExcept,
    /*! "only": listed apps go through the tunnel. NOT IMPLEMENTED - the provider
     *  refuses to claim flows in this mode instead of silently doing "except". */
    STRouteModeOnly,
};

FOUNDATION_EXPORT STRouteMode STRouteModeFromString(NSString *value);
FOUNDATION_EXPORT const char *STRouteModeName(STRouteMode mode);

/*! What the provider should do with a flow. */
typedef NS_ENUM(NSInteger, STFlowDecision) {
    /*! Return NO from handleNewFlow - the system routes it (into the tunnel). */
    STFlowDecisionSystem = 0,
    /*! Claim the flow and send it out of the physical interface. */
    STFlowDecisionBypass,
};

@interface STAppEntry : NSObject
@property (nonatomic, copy) NSString *bundleId;
@property (nonatomic, copy) NSString *path;
@end

/*!
 * Thread safety: `apps` and `mode` are atomic properties holding immutable
 * values. replaceApps:/setMode: publish a new immutable snapshot; readers on
 * arbitrary flow queues always observe a consistent one. Never mutate the
 * array returned by `apps`.
 */
@interface STAppPolicy : NSObject

@property (atomic, copy, readonly) NSArray<STAppEntry *> *apps;
@property (atomic, assign) STRouteMode mode;

- (void)replaceApps:(NSArray<STAppEntry *> *)apps;

/*! Decides what to do with a flow. `reason` is filled with a short
 *  human-readable explanation for the log. */
- (STFlowDecision)decisionForSigningId:(NSString *)signingId
                                  path:(NSString *)path
                                reason:(NSString *__autoreleasing *)reason;

@end

#endif
