#ifndef AMNEZIA_SPLIT_TUNNEL_APP_POLICY_H
#define AMNEZIA_SPLIT_TUNNEL_APP_POLICY_H

#import <Foundation/Foundation.h>

/*! Route mode sent by the host app in the "mode" key. */
typedef NS_ENUM(NSInteger, STRouteMode) {
    /*! Unknown/unset - the provider claims nothing and lets the system route. */
    STRouteModeUnknown = 0,
    /*! "except": listed apps bypass the tunnel, everything else stays in it. */
    STRouteModeExcept,
    /*! "only": listed apps stay in the tunnel, everything else bypasses it. */
    STRouteModeOnly,
};

FOUNDATION_EXPORT STRouteMode STRouteModeFromString(NSString *value);
FOUNDATION_EXPORT const char *STRouteModeName(STRouteMode mode);

/*! What the provider should do with a flow. */
typedef NS_ENUM(NSInteger, STFlowDecision) {
    /*! Return NO from handleNewFlow - the system routes it, i.e. into the
     *  tunnel, since the VPN owns the default route in both modes. */
    STFlowDecisionSystem = 0,
    /*! Claim the flow and re-open it on the physical interface. */
    STFlowDecisionBypass,
};

@interface STAppEntry : NSObject
@property (nonatomic, copy) NSString *bundleId;
@property (nonatomic, copy) NSString *path;
@end

/*!
 * Decides, per flow, whether it must be pulled out of the tunnel.
 *
 * Both modes leave the VPN's default route alone and differ only in which side
 * of the list gets claimed:
 *
 *   except: listed -> Bypass,  everything else -> System (tunnel)
 *   only:   listed -> System (tunnel),  everything else -> Bypass
 *
 * "only" therefore claims almost every flow on the machine, which makes the
 * self-exclusion below mandatory: without it the provider would claim its own
 * relay sockets and the traffic of the VPN helpers (tun2socks, amneziawg-go,
 * openvpn, AmneziaVPN-service) and recurse into itself.
 *
 * Thread safety: all properties are atomic and hold immutable values;
 * replaceApps:/setMode: publish a new snapshot that readers on arbitrary flow
 * queues observe consistently. Never mutate the array returned by `apps`.
 */
@interface STAppPolicy : NSObject

@property (atomic, copy, readonly) NSArray<STAppEntry *> *apps;
@property (atomic, assign) STRouteMode mode;

/*! Bundle id prefix of our own app family, e.g. "org.amnezia.AmneziaVPN".
 *  Matches the app, this extension and any dotted sub-identifier. */
@property (atomic, copy) NSString *selfBundleIdPrefix;
/*! Path of the host app bundle. Covers the bundled helper executables, which
 *  carry their own signing identifiers. */
@property (atomic, copy) NSString *selfAppPath;

- (void)replaceApps:(NSArray<STAppEntry *> *)apps;

/*! YES when "only" mode has everything it needs to run safely. */
- (BOOL)isSelfExclusionUsable;

/*! Why a flow got its decision. Kept as a cheap enum rather than a formatted
 *  string: the verdict is computed for every single flow, and in "only" mode
 *  that is every connection on the machine. The log line is rendered from this
 *  only when it will actually be emitted. */
typedef NS_ENUM(NSInteger, STMatchKind) {
    STMatchKindModeUnknown = 0,
    STMatchKindSelf,
    STMatchKindUnsafeOnlyMode,
    STMatchKindListed,
    STMatchKindNotListed,
};

typedef struct {
    STFlowDecision decision;
    STMatchKind kind;
    /*! Index in the app list; meaningful only for STMatchKindListed. */
    NSUInteger index;
    /*! YES when the match was on the path rather than the bundle id. */
    BOOL matchedByPath;
    /*! Size of the list the verdict was taken against. */
    NSUInteger listCount;
} STFlowVerdict;

/*! YES when the executable path still has to be resolved for this flow, i.e.
 *  the signing identifier alone cannot settle it. Resolving the path goes
 *  through SecCodeCopyGuestWithAttributes, so it is worth skipping. */
- (BOOL)needsPathForSigningId:(NSString *)signingId;

/*! Allocation-free; call this on every flow. */
- (STFlowVerdict)verdictForSigningId:(NSString *)signingId path:(NSString *)path;

@end

/*! Renders a verdict for the log. Call only when the line is really emitted. */
FOUNDATION_EXPORT NSString *STDescribeVerdict(STFlowVerdict verdict);

#endif
