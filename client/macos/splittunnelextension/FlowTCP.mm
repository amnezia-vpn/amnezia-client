#import "FlowTCP.h"
#import "Utils.h"

#import <os/log.h>

@interface FlowTCP ()
@property (nonatomic) NEAppProxyTCPFlow *flow;
@property (nonatomic) nw_connection_t connection;
@property (nonatomic) BOOL closed;
@end

static NSMutableSet<FlowTCP *> *TCPSessions(void)
{
    static NSMutableSet<FlowTCP *> *sessions;
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        sessions = [NSMutableSet set];
    });
    return sessions;
}

@implementation FlowTCP

+ (void)handleFlow:(NEAppProxyTCPFlow *)flow interface:(nw_interface_t)interface
{
    FlowTCP *session = [[FlowTCP alloc] init];
    session.flow = flow;
    [TCPSessions() addObject:session];
    [session startWithInterface:interface];
}

- (void)startWithInterface:(nw_interface_t)interface
{
    nw_endpoint_t remote = [self copyRemoteEndpoint];
    if (remote == NULL) {
        os_log_error(STUtils.log, "tcp: missing remote endpoint");
        [self.flow closeReadWithError:nil];
        [self.flow closeWriteWithError:nil];
        return;
    }

    nw_parameters_t params = nw_parameters_create_secure_tcp(NW_PARAMETERS_DISABLE_PROTOCOL, NW_PARAMETERS_DEFAULT_CONFIGURATION);
    if (interface != NULL) {
        nw_parameters_require_interface(params, interface);
    }

    self.connection = nw_connection_create(remote, params);

    __weak FlowTCP *weakSelf = self;
    void (^opened)(NSError *) = ^(NSError *error) {
        FlowTCP *strongSelf = weakSelf;
        if (strongSelf == nil) {
            return;
        }
        if (error != nil) {
            os_log_error(STUtils.log, "tcp: open flow failed: %{public}@", error);
            [strongSelf closeWithError:error];
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
        return NULL;
    }
    NWHostEndpoint *host = (NWHostEndpoint *)endpoint;
    return [STUtils copyEndpointFromHost:host.hostname port:host.port];
}

- (void)startConnection
{
    __weak FlowTCP *weakSelf = self;
    nw_connection_set_queue(self.connection, dispatch_get_main_queue());
    nw_connection_set_state_changed_handler(self.connection, ^(nw_connection_state_t state, nw_error_t error) {
        FlowTCP *strongSelf = weakSelf;
        if (strongSelf == nil) {
            return;
        }
        if (state == nw_connection_state_ready) {
            [strongSelf copyFlowToConnection];
            [strongSelf copyConnectionToFlow];
        } else if (state == nw_connection_state_failed || state == nw_connection_state_cancelled) {
            NSError *nsError = nil;
            if (error != NULL) {
                nsError = CFBridgingRelease(nw_error_copy_cf_error(error));
            }
            [strongSelf closeWithError:nsError];
        }
    });
    nw_connection_start(self.connection);
}

- (void)copyFlowToConnection
{
    __weak FlowTCP *weakSelf = self;
    [self.flow readDataWithCompletionHandler:^(NSData *data, NSError *error) {
        FlowTCP *strongSelf = weakSelf;
        if (strongSelf == nil || strongSelf.closed) {
            return;
        }
        if (error != nil || data == nil) {
            [strongSelf closeWithError:error];
            return;
        }
        if (data.length == 0) {
            nw_connection_send(strongSelf.connection, NULL, NW_CONNECTION_DEFAULT_MESSAGE_CONTEXT, true,
                               ^(nw_error_t sendError) {
                                   (void)sendError;
                               });
            [strongSelf closeWithError:nil];
            return;
        }

        dispatch_data_t payload = dispatch_data_create(data.bytes, data.length, dispatch_get_main_queue(), DISPATCH_DATA_DESTRUCTOR_DEFAULT);
        nw_connection_send(strongSelf.connection, payload, NW_CONNECTION_DEFAULT_MESSAGE_CONTEXT, true, ^(nw_error_t sendError) {
            FlowTCP *inner = weakSelf;
            if (inner == nil) {
                return;
            }
            if (sendError != NULL) {
                [inner closeWithError:CFBridgingRelease(nw_error_copy_cf_error(sendError))];
                return;
            }
            [inner copyFlowToConnection];
        });
    }];
}

- (void)copyConnectionToFlow
{
    __weak FlowTCP *weakSelf = self;
    nw_connection_receive(self.connection, 1, UINT32_MAX,
                          ^(dispatch_data_t content, nw_content_context_t context, bool isComplete, nw_error_t error) {
                              (void)context;
                              FlowTCP *strongSelf = weakSelf;
                              if (strongSelf == nil || strongSelf.closed) {
                                  return;
                              }
                              if (error != NULL) {
                                  [strongSelf closeWithError:CFBridgingRelease(nw_error_copy_cf_error(error))];
                                  return;
                              }

                              NSData *data = [STUtils dataFromDispatchData:content];

                              if (data.length > 0) {
                                  [strongSelf.flow writeData:data withCompletionHandler:^(NSError *writeError) {
                                      FlowTCP *inner = weakSelf;
                                      if (inner == nil) {
                                          return;
                                      }
                                      if (writeError != nil) {
                                          [inner closeWithError:writeError];
                                          return;
                                      }
                                      if (!isComplete) {
                                          [inner copyConnectionToFlow];
                                      } else {
                                          [inner closeWithError:nil];
                                      }
                                  }];
                                  return;
                              }

                              if (isComplete) {
                                  [strongSelf closeWithError:nil];
                                  return;
                              }
                              [strongSelf copyConnectionToFlow];
                          });
}

- (void)closeWithError:(NSError *)error
{
    if (self.closed) {
        return;
    }
    self.closed = YES;
    [self.flow closeReadWithError:error];
    [self.flow closeWriteWithError:error];
    if (self.connection != NULL) {
        nw_connection_cancel(self.connection);
        self.connection = NULL;
    }
    [TCPSessions() removeObject:self];
}

@end
