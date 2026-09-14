#import "FlowUDP.h"
#import "Utils.h"

#import <os/log.h>

@interface FlowUDP ()
@property (atomic) NEAppProxyUDPFlow *flow;
@property (atomic) nw_interface_t interface;
@property (atomic) BOOL closed;
@property (atomic) uint64_t flowId;
@property (atomic) uint64_t datagramsOut;
@property (atomic) uint64_t datagramsIn;
/*! Guarded by STUtils.stateQueue. */
@property (nonatomic) NSMutableDictionary<NSString *, nw_connection_t> *connections;
@end

/*! Registry keeping sessions alive while they are running.
 *  MUST only be touched on STUtils.stateQueue. */
static NSMutableSet<FlowUDP *> *UDPSessionsLocked(void)
{
    static NSMutableSet<FlowUDP *> *sessions;
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        sessions = [NSMutableSet set];
    });
    return sessions;
}

@implementation FlowUDP

+ (void)handleFlow:(NEAppProxyUDPFlow *)flow interface:(nw_interface_t)interface flowId:(uint64_t)flowId
{
    FlowUDP *session = [[FlowUDP alloc] init];
    session.flow = flow;
    session.interface = interface;
    session.flowId = flowId;
    session.connections = [NSMutableDictionary dictionary];

    dispatch_sync([STUtils stateQueue], ^{
        [UDPSessionsLocked() addObject:session];
        STLogDebug("udp[%{public}llu]: registered, %{public}lu live udp sessions",
                   flowId, (unsigned long)UDPSessionsLocked().count);
    });

    [session start];
}

- (void)start
{
    const uint64_t fid = self.flowId;
    STLogInfo("udp[%{public}llu]: opening flow, pinned if=%{public}s", fid,
              self.interface != NULL ? (nw_interface_get_name(self.interface) ?: "?") : "(default)");
    if (self.interface == NULL) {
        STLogError("udp[%{public}llu]: no interface pinned - the bypass may leak back into the tunnel", fid);
    }

    __weak FlowUDP *weakSelf = self;
    void (^opened)(NSError *) = ^(NSError *error) {
        FlowUDP *strongSelf = weakSelf;
        if (strongSelf == nil) {
            return;
        }
        if (error != nil) {
            STLogError("udp[%{public}llu]: open flow failed: %{public}@", fid, error);
            [strongSelf closeWithError:error stage:@"open-flow"];
            return;
        }
        STLogDebug("udp[%{public}llu]: flow opened", fid);
        [strongSelf readDatagrams];
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

- (NSString *)keyForEndpoint:(NWHostEndpoint *)endpoint
{
    return [NSString stringWithFormat:@"%@:%@", endpoint.hostname, endpoint.port];
}

#pragma mark - app -> remote

- (void)readDatagrams
{
    const uint64_t fid = self.flowId;
    __weak FlowUDP *weakSelf = self;
    [self.flow readDatagramsWithCompletionHandler:^(NSArray<NSData *> *datagrams, NSArray<NWEndpoint *> *remoteEndpoints, NSError *error) {
        FlowUDP *strongSelf = weakSelf;
        if (strongSelf == nil || strongSelf.closed) {
            return;
        }
        if (error != nil) {
            STLogError("udp[%{public}llu]: flow read failed: %{public}@", fid, error);
            [strongSelf closeWithError:error stage:@"flow-read"];
            return;
        }
        if (datagrams.count == 0) {
            STLogInfo("udp[%{public}llu]: flow reported end of datagrams", fid);
            [strongSelf closeWithError:nil stage:@"flow-eof"];
            return;
        }

        const NSUInteger count = MIN(datagrams.count, remoteEndpoints.count);
        if (datagrams.count != remoteEndpoints.count) {
            STLogError("udp[%{public}llu]: datagram/endpoint count mismatch %{public}lu vs %{public}lu",
                       fid, (unsigned long)datagrams.count, (unsigned long)remoteEndpoints.count);
        }
        STLogDebug("udp[%{public}llu]: app -> remote batch of %{public}lu datagrams", fid, (unsigned long)count);

        for (NSUInteger i = 0; i < count; i++) {
            NWEndpoint *endpoint = remoteEndpoints[i];
            if (![endpoint isKindOfClass:[NWHostEndpoint class]]) {
                STLogError("udp[%{public}llu]: endpoint[%{public}lu] is %{public}@, skipped",
                           fid, (unsigned long)i, [endpoint class]);
                continue;
            }
            [strongSelf sendDatagram:datagrams[i] toHost:(NWHostEndpoint *)endpoint];
        }
        [strongSelf readDatagrams];
    }];
}

/*! Returns the connection for `host`, creating and starting it if needed.
 *  All access to the connection map happens on STUtils.stateQueue. */
- (nw_connection_t)connectionForHost:(NWHostEndpoint *)host
{
    const uint64_t fid = self.flowId;
    NSString *key = [self keyForEndpoint:host];
    __block nw_connection_t result = NULL;
    __block BOOL created = NO;

    dispatch_sync([STUtils stateQueue], ^{
        if (self.closed) {
            return;
        }
        result = self.connections[key];
        if (result != NULL) {
            return;
        }
        nw_endpoint_t remote = [STUtils copyEndpointFromHost:host.hostname port:host.port];
        if (remote == NULL) {
            STLogError("udp[%{public}llu]: cannot build endpoint for %{public}@", fid, key);
            return;
        }
        nw_parameters_t params = nw_parameters_create_secure_udp(NW_PARAMETERS_DISABLE_PROTOCOL,
                                                                 NW_PARAMETERS_DEFAULT_CONFIGURATION);
        if (self.interface != NULL) {
            nw_parameters_require_interface(params, self.interface);
        }
        result = nw_connection_create(remote, params);
        self.connections[key] = result;
        created = YES;
        STLogInfo("udp[%{public}llu]: new remote connection %{public}@ (%{public}lu open)",
                  fid, key, (unsigned long)self.connections.count);
    });

    if (!created || result == NULL) {
        return result;
    }

    __weak FlowUDP *weakSelf = self;
    nw_connection_t connection = result;
    nw_connection_set_queue(connection, dispatch_get_main_queue());
    nw_connection_set_state_changed_handler(connection, ^(nw_connection_state_t state, nw_error_t error) {
        FlowUDP *strongSelf = weakSelf;
        if (strongSelf == nil) {
            return;
        }
        NSError *nsError = [STUtils errorFromNWError:error];
        STLogDebug("udp[%{public}llu]: %{public}@ state=%{public}s error=%{public}@",
                   fid, key, [STUtils connectionStateName:state], nsError ?: @"nil");
        if (state == nw_connection_state_ready) {
            [strongSelf receiveFromConnection:connection host:host key:key];
        } else if (state == nw_connection_state_failed || state == nw_connection_state_cancelled) {
            if (state == nw_connection_state_failed) {
                STLogError("udp[%{public}llu]: %{public}@ failed: %{public}@", fid, key, nsError ?: @"nil");
            }
            [strongSelf forgetConnectionForKey:key cancel:(state == nw_connection_state_failed)];
        }
    });
    nw_connection_start(connection);
    return connection;
}

- (void)forgetConnectionForKey:(NSString *)key cancel:(BOOL)cancel
{
    const uint64_t fid = self.flowId;
    dispatch_async([STUtils stateQueue], ^{
        nw_connection_t connection = self.connections[key];
        if (connection == NULL) {
            return;
        }
        [self.connections removeObjectForKey:key];
        if (cancel) {
            nw_connection_cancel(connection);
        }
        STLogDebug("udp[%{public}llu]: dropped remote connection %{public}@ (%{public}lu left)",
                   fid, key, (unsigned long)self.connections.count);
    });
}

- (void)sendDatagram:(NSData *)datagram toHost:(NWHostEndpoint *)host
{
    const uint64_t fid = self.flowId;
    nw_connection_t connection = [self connectionForHost:host];
    if (connection == NULL) {
        STLogError("udp[%{public}llu]: no connection for %{public}@:%{public}@, datagram dropped",
                   fid, host.hostname, host.port);
        return;
    }

    self.datagramsOut = self.datagramsOut + 1;
    STLogDebug("udp[%{public}llu]: app -> %{public}@:%{public}@ %{public}lu bytes (total out %{public}llu)",
               fid, host.hostname, host.port, (unsigned long)datagram.length, self.datagramsOut);

    dispatch_data_t payload = dispatch_data_create(datagram.bytes, datagram.length, dispatch_get_main_queue(),
                                                   DISPATCH_DATA_DESTRUCTOR_DEFAULT);
    nw_connection_send(connection, payload, NW_CONNECTION_DEFAULT_MESSAGE_CONTEXT, true, ^(nw_error_t sendError) {
        NSError *nsError = [STUtils errorFromNWError:sendError];
        if (nsError != nil) {
            STLogError("udp[%{public}llu]: send to %{public}@:%{public}@ failed: %{public}@",
                       fid, host.hostname, host.port, nsError);
        }
    });
}

#pragma mark - remote -> app

- (void)receiveFromConnection:(nw_connection_t)connection host:(NWHostEndpoint *)host key:(NSString *)key
{
    const uint64_t fid = self.flowId;
    __weak FlowUDP *weakSelf = self;
    nw_connection_receive(connection, 1, UINT32_MAX,
                          ^(dispatch_data_t content, nw_content_context_t context, bool isComplete, nw_error_t error) {
                              (void)context;
                              FlowUDP *strongSelf = weakSelf;
                              if (strongSelf == nil || strongSelf.closed) {
                                  return;
                              }
                              NSError *nsError = [STUtils errorFromNWError:error];
                              if (nsError != nil) {
                                  STLogError("udp[%{public}llu]: receive from %{public}@ failed: %{public}@",
                                             fid, key, nsError);
                                  [strongSelf forgetConnectionForKey:key cancel:YES];
                                  return;
                              }
                              if (content != NULL) {
                                  NSData *data = [STUtils dataFromDispatchData:content];
                                  strongSelf.datagramsIn = strongSelf.datagramsIn + 1;
                                  STLogDebug("udp[%{public}llu]: %{public}@ -> app %{public}lu bytes (total in %{public}llu)",
                                             fid, key, (unsigned long)data.length, strongSelf.datagramsIn);
                                  [strongSelf.flow writeDatagrams:@[ data ]
                                                  sentByEndpoints:@[ host ]
                                                completionHandler:^(NSError *writeError) {
                                                    if (writeError != nil) {
                                                        STLogError("udp[%{public}llu]: flow write failed: %{public}@",
                                                                   fid, writeError);
                                                    }
                                                }];
                              }
                              if (isComplete) {
                                  STLogDebug("udp[%{public}llu]: %{public}@ marked complete", fid, key);
                                  [strongSelf forgetConnectionForKey:key cancel:YES];
                                  return;
                              }
                              [strongSelf receiveFromConnection:connection host:host key:key];
                          });
}

#pragma mark - teardown

- (void)closeWithError:(NSError *)error stage:(NSString *)stage
{
    FlowUDP *keepAlive = self;
    @synchronized(keepAlive) {
        if (keepAlive.closed) {
            return;
        }
        keepAlive.closed = YES;
    }

    STLogInfo("udp[%{public}llu]: closing at %{public}@ out=%{public}llu in=%{public}llu error=%{public}@",
              keepAlive.flowId, stage ?: @"?", keepAlive.datagramsOut, keepAlive.datagramsIn, error ?: @"nil");

    [keepAlive.flow closeReadWithError:error];
    [keepAlive.flow closeWriteWithError:error];

    dispatch_async([STUtils stateQueue], ^{
        for (nw_connection_t connection in keepAlive.connections.allValues) {
            nw_connection_cancel(connection);
        }
        [keepAlive.connections removeAllObjects];
        [UDPSessionsLocked() removeObject:keepAlive];
        STLogDebug("udp[%{public}llu]: unregistered, %{public}lu live udp sessions",
                   keepAlive.flowId, (unsigned long)UDPSessionsLocked().count);
    });
}

@end
