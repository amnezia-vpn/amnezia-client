#import "FlowTCP.h"
#import "Utils.h"

#import <os/log.h>

@interface FlowTCP ()
@property (atomic) NEAppProxyTCPFlow *flow;
@property (atomic) nw_connection_t connection;
@property (atomic) BOOL closed;
@property (atomic) uint64_t flowId;
@property (atomic) uint64_t bytesOut;
@property (atomic) uint64_t bytesIn;
@property (atomic) BOOL flowEofSeen;
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

    dispatch_sync([STUtils stateQueue], ^{
        [TCPSessionsLocked() addObject:session];
        STLogDebug("tcp[%{public}llu]: registered, %{public}lu live tcp sessions",
                   flowId, (unsigned long)TCPSessionsLocked().count);
    });

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

    STLogInfo("tcp[%{public}llu]: connecting to %{public}s:%{public}u via %{public}s",
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
        STLogDebug("tcp[%{public}llu]: flow opened", fid);
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
    nw_connection_set_queue(self.connection, dispatch_get_main_queue());
    nw_connection_set_state_changed_handler(self.connection, ^(nw_connection_state_t state, nw_error_t error) {
        FlowTCP *strongSelf = weakSelf;
        if (strongSelf == nil) {
            return;
        }
        NSError *nsError = [STUtils errorFromNWError:error];
        STLogDebug("tcp[%{public}llu]: connection state=%{public}s error=%{public}@",
                   fid, [STUtils connectionStateName:state], nsError ?: @"nil");
        if (state == nw_connection_state_ready) {
            STLogInfo("tcp[%{public}llu]: connection ready, starting relay", fid);
            [strongSelf copyFlowToConnection];
            [strongSelf copyConnectionToFlow];
        } else if (state == nw_connection_state_failed) {
            STLogError("tcp[%{public}llu]: connection failed: %{public}@", fid, nsError ?: @"nil");
            [strongSelf closeWithError:nsError stage:@"conn-failed"];
        } else if (state == nw_connection_state_cancelled) {
            [strongSelf closeWithError:nsError stage:@"conn-cancelled"];
        }
    });
    nw_connection_start(self.connection);
}

#pragma mark - app -> remote

- (void)copyFlowToConnection
{
    const uint64_t fid = self.flowId;
    __weak FlowTCP *weakSelf = self;
    [self.flow readDataWithCompletionHandler:^(NSData *data, NSError *error) {
        FlowTCP *strongSelf = weakSelf;
        if (strongSelf == nil || strongSelf.closed) {
            return;
        }
        if (error != nil) {
            STLogError("tcp[%{public}llu]: flow read failed: %{public}@", fid, error);
            [strongSelf closeWithError:error stage:@"flow-read"];
            return;
        }
        if (data == nil) {
            STLogDebug("tcp[%{public}llu]: flow read returned nil", fid);
            [strongSelf closeWithError:nil stage:@"flow-read-nil"];
            return;
        }

        if (data.length == 0) {
            // The app half-closed its side. Propagate a real FIN with the final
            // message context and keep reading the response until the remote is
            // done - do NOT cancel the connection here.
            strongSelf.flowEofSeen = YES;
            STLogInfo("tcp[%{public}llu]: app half-closed after %{public}llu bytes out, sending FIN",
                      fid, strongSelf.bytesOut);
            nw_connection_send(strongSelf.connection, NULL, NW_CONNECTION_FINAL_MESSAGE_CONTEXT, true,
                               ^(nw_error_t sendError) {
                                   NSError *nsError = [STUtils errorFromNWError:sendError];
                                   if (nsError != nil) {
                                       STLogError("tcp[%{public}llu]: FIN send failed: %{public}@", fid, nsError);
                                   } else {
                                       STLogDebug("tcp[%{public}llu]: FIN sent", fid);
                                   }
                               });
            return;
        }

        strongSelf.bytesOut = strongSelf.bytesOut + data.length;
        STLogDebug("tcp[%{public}llu]: app -> remote %{public}lu bytes (total %{public}llu)",
                   fid, (unsigned long)data.length, strongSelf.bytesOut);

        dispatch_data_t payload = dispatch_data_create(data.bytes, data.length, dispatch_get_main_queue(),
                                                       DISPATCH_DATA_DESTRUCTOR_DEFAULT);
        nw_connection_send(strongSelf.connection, payload, NW_CONNECTION_DEFAULT_MESSAGE_CONTEXT, true,
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
                               [inner copyFlowToConnection];
                           });
    }];
}

#pragma mark - remote -> app

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
                                  STLogDebug("tcp[%{public}llu]: remote -> app %{public}lu bytes (total %{public}llu, complete=%{public}d)",
                                             fid, (unsigned long)data.length, strongSelf.bytesIn, (int)isComplete);
                                  [strongSelf.flow writeData:data withCompletionHandler:^(NSError *writeError) {
                                      FlowTCP *inner = weakSelf;
                                      if (inner == nil || inner.closed) {
                                          return;
                                      }
                                      if (writeError != nil) {
                                          STLogError("tcp[%{public}llu]: flow write failed: %{public}@", fid, writeError);
                                          [inner closeWithError:writeError stage:@"flow-write"];
                                          return;
                                      }
                                      if (isComplete) {
                                          STLogInfo("tcp[%{public}llu]: remote finished the stream", fid);
                                          [inner closeWithError:nil stage:@"remote-eof"];
                                      } else {
                                          [inner copyConnectionToFlow];
                                      }
                                  }];
                                  return;
                              }

                              if (isComplete) {
                                  STLogInfo("tcp[%{public}llu]: remote finished the stream (no trailing data)", fid);
                                  [strongSelf closeWithError:nil stage:@"remote-eof-empty"];
                                  return;
                              }
                              [strongSelf copyConnectionToFlow];
                          });
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

    STLogInfo("tcp[%{public}llu]: closing at %{public}@ out=%{public}llu in=%{public}llu eofSeen=%{public}d error=%{public}@",
              keepAlive.flowId, stage ?: @"?", keepAlive.bytesOut, keepAlive.bytesIn,
              (int)keepAlive.flowEofSeen, error ?: @"nil");

    [keepAlive.flow closeReadWithError:error];
    [keepAlive.flow closeWriteWithError:error];

    nw_connection_t connection = keepAlive.connection;
    if (connection != NULL) {
        nw_connection_cancel(connection);
        keepAlive.connection = NULL;
    }

    dispatch_async([STUtils stateQueue], ^{
        [TCPSessionsLocked() removeObject:keepAlive];
        STLogDebug("tcp[%{public}llu]: unregistered, %{public}lu live tcp sessions",
                   keepAlive.flowId, (unsigned long)TCPSessionsLocked().count);
    });
}

@end
