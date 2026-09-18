#import "FlowTCP.h"
#import "Utils.h"

#import <os/log.h>

namespace {
/*! Stop reading from the app once this many bytes are waiting to be written to
 *  the remote, and resume once the backlog drops below the low-water mark.
 *  Without this the relay would read as fast as the app can produce. */
const uint64_t kSendHighWater = 256 * 1024;
const uint64_t kSendLowWater = 64 * 1024;
} // namespace

@interface FlowTCP ()
@property (atomic) NEAppProxyTCPFlow *flow;
@property (atomic) nw_connection_t connection;
@property (atomic) BOOL closed;
@property (atomic) uint64_t flowId;
@property (atomic) uint64_t bytesOut;
@property (atomic) uint64_t bytesIn;
@property (atomic) BOOL flowEofSeen;

/*! Every callback of this session runs here, so one slow flow cannot stall the
 *  others. All the fields below are owned by this queue. */
@property (nonatomic) dispatch_queue_t queue;
@property (nonatomic) uint64_t pendingSendBytes;
@property (nonatomic) BOOL readPaused;
@property (nonatomic) NSMutableArray<NSData *> *writeBacklog;
@property (nonatomic) BOOL writeInFlight;
- (void)closeWithError:(NSError *)error stage:(NSString *)stage;
@property (nonatomic) BOOL remoteEofSeen;
@end

/*! Registry keeping sessions alive while they are running.
 *  MUST only be touched on STUtils.stateQueue. */
static NSMutableSet<FlowTCP *> *TCPSessionsLocked(void)
{
    static NSMutableSet<FlowTCP *> *sessions;
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        sessions = [NSMutableSet set];
    });
    return sessions;
}

@implementation FlowTCP

+ (void)handleFlow:(NEAppProxyTCPFlow *)flow interface:(nw_interface_t)interface flowId:(uint64_t)flowId
{
    FlowTCP *session = [[FlowTCP alloc] init];
    session.flow = flow;
    session.flowId = flowId;
    session.writeBacklog = [NSMutableArray array];

    char label[64];
    snprintf(label, sizeof(label), "org.amnezia.split-tunnel.tcp.%llu", (unsigned long long)flowId);
    session.queue = dispatch_queue_create(label, DISPATCH_QUEUE_SERIAL);

    dispatch_sync([STUtils stateQueue], ^{
        [TCPSessionsLocked() addObject:session];
    });
    [STStats tcpOpened];

    [session startWithInterface:interface];
}

- (void)startWithInterface:(nw_interface_t)interface
{
    const uint64_t fid = self.flowId;
    nw_endpoint_t remote = [self copyRemoteEndpoint];
    if (remote == NULL) {
        STLogError("tcp[%{public}llu]: missing or unusable remote endpoint, dropping flow", fid);
        [self closeWithError:nil stage:@"no-remote"];
        return;
    }

    STLogDebug("tcp[%{public}llu]: connecting to %{public}s:%{public}u via %{public}s",
               fid,
               nw_endpoint_get_hostname(remote) ?: "?",
               (unsigned)nw_endpoint_get_port(remote),
               interface != NULL ? (nw_interface_get_name(interface) ?: "?") : "(default)");

    nw_parameters_t params = nw_parameters_create_secure_tcp(NW_PARAMETERS_DISABLE_PROTOCOL, NW_PARAMETERS_DEFAULT_CONFIGURATION);
    if (interface != NULL) {
        nw_parameters_require_interface(params, interface);
    } else {
        STLogError("tcp[%{public}llu]: no interface pinned - the bypass may leak back into the tunnel", fid);
    }

    self.connection = nw_connection_create(remote, params);

    __weak FlowTCP *weakSelf = self;
    void (^opened)(NSError *) = ^(NSError *error) {
        FlowTCP *strongSelf = weakSelf;
        if (strongSelf == nil) {
            return;
        }
        if (error != nil) {
            STLogError("tcp[%{public}llu]: open flow failed: %{public}@", fid, error);
            [strongSelf closeWithError:error stage:@"open-flow"];
            return;
        }
        [strongSelf startConnection];
    };

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    if (@available(macOS 15.0, *)) {
        [self.flow openWithLocalFlowEndpoint:nil completionHandler:opened];
    } else {
        [self.flow openWithLocalEndpoint:nil completionHandler:opened];
    }
#pragma clang diagnostic pop
}

- (nw_endpoint_t)copyRemoteEndpoint
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    NWEndpoint *endpoint = self.flow.remoteEndpoint;
#pragma clang diagnostic pop
    if (![endpoint isKindOfClass:[NWHostEndpoint class]]) {
        STLogError("tcp[%{public}llu]: remote endpoint is %{public}@, expected NWHostEndpoint",
                   self.flowId, [endpoint class]);
        return NULL;
    }
    NWHostEndpoint *host = (NWHostEndpoint *)endpoint;
    return [STUtils copyEndpointFromHost:host.hostname port:host.port];
}

- (void)startConnection
{
    const uint64_t fid = self.flowId;
    __weak FlowTCP *weakSelf = self;
    nw_connection_set_queue(self.connection, self.queue);
    nw_connection_set_state_changed_handler(self.connection, ^(nw_connection_state_t state, nw_error_t error) {
        FlowTCP *strongSelf = weakSelf;
        if (strongSelf == nil) {
            return;
        }
        NSError *nsError = [STUtils errorFromNWError:error];
        STLogDebug("tcp[%{public}llu]: state=%{public}s error=%{public}@",
                   fid, [STUtils connectionStateName:state], nsError ?: @"nil");
        if (state == nw_connection_state_waiting) {
            // "waiting" with no error means the system found no usable path for
            // the interface this flow is pinned to; the path itself says why.
            STLogInfo("tcp[%{public}llu]: waiting - %{public}@",
                      fid, [STUtils describeConnectionPath:strongSelf.connection]);
        }
        if (state == nw_connection_state_ready) {
            STLogDebug("tcp[%{public}llu]: connection ready, starting relay", fid);
            [strongSelf copyFlowToConnection];
            [strongSelf copyConnectionToFlow];
        } else if (state == nw_connection_state_failed) {
            STLogError("tcp[%{public}llu]: failed - %{public}@",
                       fid, [STUtils describeConnectionPath:strongSelf.connection]);
            STLogError("tcp[%{public}llu]: connection failed: %{public}@", fid, nsError ?: @"nil");
            [strongSelf closeWithError:nsError stage:@"conn-failed"];
        } else if (state == nw_connection_state_cancelled) {
            [strongSelf closeWithError:nsError stage:@"conn-cancelled"];
        }
    });
    nw_connection_start(self.connection);
}

#pragma mark - app -> remote

/*! Issues one read. The next read is started as soon as this one returns, i.e.
 *  without waiting for the send to complete - the send is what has to cross the
 *  network, and waiting for it turned the relay into stop-and-wait. Ordering is
 *  still guaranteed: sends on one connection are delivered in the order issued. */
- (void)copyFlowToConnection
{
    const uint64_t fid = self.flowId;
    __weak FlowTCP *weakSelf = self;
    [self.flow readDataWithCompletionHandler:^(NSData *data, NSError *error) {
        FlowTCP *strongSelf = weakSelf;
        if (strongSelf == nil || strongSelf.closed) {
            return;
        }
        dispatch_async(strongSelf.queue, ^{
            FlowTCP *self2 = weakSelf;
            if (self2 == nil || self2.closed) {
                return;
            }
            if (error != nil) {
                STLogError("tcp[%{public}llu]: flow read failed: %{public}@", fid, error);
                [self2 closeWithError:error stage:@"flow-read"];
                return;
            }
            if (data == nil) {
                [self2 closeWithError:nil stage:@"flow-read-nil"];
                return;
            }

            if (data.length == 0) {
                // The app half-closed its side. Propagate a real FIN and keep
                // reading the response - do NOT cancel the connection here.
                self2.flowEofSeen = YES;
                STLogDebug("tcp[%{public}llu]: app half-closed after %{public}llu bytes out, sending FIN",
                           fid, self2.bytesOut);
                nw_connection_send(self2.connection, NULL, NW_CONNECTION_FINAL_MESSAGE_CONTEXT, true,
                                   ^(nw_error_t sendError) {
                                       NSError *nsError = [STUtils errorFromNWError:sendError];
                                       if (nsError != nil) {
                                           STLogError("tcp[%{public}llu]: FIN send failed: %{public}@", fid, nsError);
                                       }
                                   });
                return;
            }

            const uint64_t length = data.length;
            self2.bytesOut = self2.bytesOut + length;
            self2.pendingSendBytes += length;

            dispatch_data_t payload = dispatch_data_create(data.bytes, data.length, self2.queue,
                                                           DISPATCH_DATA_DESTRUCTOR_DEFAULT);
            nw_connection_send(self2.connection, payload, NW_CONNECTION_DEFAULT_MESSAGE_CONTEXT, true,
                               ^(nw_error_t sendError) {
                                   FlowTCP *inner = weakSelf;
                                   if (inner == nil || inner.closed) {
                                       return;
                                   }
                                   NSError *nsError = [STUtils errorFromNWError:sendError];
                                   if (nsError != nil) {
                                       STLogError("tcp[%{public}llu]: remote send failed: %{public}@", fid, nsError);
                                       [inner closeWithError:nsError stage:@"remote-send"];
                                       return;
                                   }
                                   inner.pendingSendBytes -= MIN(inner.pendingSendBytes, length);
                                   if (inner.readPaused && inner.pendingSendBytes <= kSendLowWater) {
                                       inner.readPaused = NO;
                                       STLogDebug("tcp[%{public}llu]: resuming reads, backlog %{public}llu",
                                                  fid, inner.pendingSendBytes);
                                       [inner copyFlowToConnection];
                                   }
                               });

            if (self2.pendingSendBytes >= kSendHighWater) {
                self2.readPaused = YES;
                STLogDebug("tcp[%{public}llu]: pausing reads, backlog %{public}llu",
                           fid, self2.pendingSendBytes);
            } else {
                [self2 copyFlowToConnection];
            }
        });
    }];
}

#pragma mark - remote -> app

/*! Same idea in the other direction: the next receive is issued immediately and
 *  the writes to the flow are drained through a backlog, because NEAppProxyFlow
 *  allows only one outstanding write. */
- (void)copyConnectionToFlow
{
    const uint64_t fid = self.flowId;
    __weak FlowTCP *weakSelf = self;
    nw_connection_receive(self.connection, 1, UINT32_MAX,
                          ^(dispatch_data_t content, nw_content_context_t context, bool isComplete, nw_error_t error) {
                              (void)context;
                              FlowTCP *strongSelf = weakSelf;
                              if (strongSelf == nil || strongSelf.closed) {
                                  return;
                              }
                              NSError *nsError = [STUtils errorFromNWError:error];
                              if (nsError != nil) {
                                  STLogError("tcp[%{public}llu]: remote receive failed: %{public}@", fid, nsError);
                                  [strongSelf closeWithError:nsError stage:@"remote-receive"];
                                  return;
                              }

                              NSData *data = [STUtils dataFromDispatchData:content];
                              if (data.length > 0) {
                                  strongSelf.bytesIn = strongSelf.bytesIn + data.length;
                                  [strongSelf.writeBacklog addObject:data];
                                  [strongSelf drainWriteBacklog];
                              }

                              if (isComplete) {
                                  STLogDebug("tcp[%{public}llu]: remote finished the stream", fid);
                                  strongSelf.remoteEofSeen = YES;
                                  [strongSelf drainWriteBacklog];
                                  return;
                              }
                              [strongSelf copyConnectionToFlow];
                          });
}

- (void)drainWriteBacklog
{
    if (self.writeInFlight || self.closed) {
        return;
    }
    if (self.writeBacklog.count == 0) {
        if (self.remoteEofSeen) {
            [self closeWithError:nil stage:@"remote-eof"];
        }
        return;
    }

    NSData *chunk = self.writeBacklog.firstObject;
    [self.writeBacklog removeObjectAtIndex:0];
    self.writeInFlight = YES;

    const uint64_t fid = self.flowId;
    __weak FlowTCP *weakSelf = self;
    [self.flow writeData:chunk withCompletionHandler:^(NSError *writeError) {
        FlowTCP *strongSelf = weakSelf;
        if (strongSelf == nil) {
            return;
        }
        dispatch_async(strongSelf.queue, ^{
            FlowTCP *self2 = weakSelf;
            if (self2 == nil) {
                return;
            }
            self2.writeInFlight = NO;
            if (writeError != nil) {
                STLogError("tcp[%{public}llu]: flow write failed: %{public}@", fid, writeError);
                [self2 closeWithError:writeError stage:@"flow-write"];
                return;
            }
            [self2 drainWriteBacklog];
        });
    }];
}

+ (void)closeAll
{
    __block NSArray<FlowTCP *> *live = nil;
    dispatch_sync([STUtils stateQueue], ^{
        live = [TCPSessionsLocked() allObjects];
    });
    if (live.count > 0) {
        STLogInfo("tcp: closing %{public}lu live session(s) on stop", (unsigned long)live.count);
    }
    for (FlowTCP *session in live) {
        [session closeWithError:nil stage:@"proxy-stopped"];
    }
}

#pragma mark - teardown

- (void)closeWithError:(NSError *)error stage:(NSString *)stage
{
    // Keep ourselves alive: removing from the registry may drop the last reference.
    FlowTCP *keepAlive = self;
    @synchronized(keepAlive) {
        if (keepAlive.closed) {
            return;
        }
        keepAlive.closed = YES;
    }

    STLogDebug("tcp[%{public}llu]: closing at %{public}@ out=%{public}llu in=%{public}llu error=%{public}@",
               keepAlive.flowId, stage ?: @"?", keepAlive.bytesOut, keepAlive.bytesIn, error ?: @"nil");

    [keepAlive.flow closeReadWithError:error];
    [keepAlive.flow closeWriteWithError:error];

    nw_connection_t connection = keepAlive.connection;
    if (connection != NULL) {
        nw_connection_cancel(connection);
        keepAlive.connection = NULL;
    }

    [STStats tcpClosedWithOut:keepAlive.bytesOut in:keepAlive.bytesIn];
    dispatch_async([STUtils stateQueue], ^{
        [TCPSessionsLocked() removeObject:keepAlive];
    });
}

@end
