#import "FlowUDP.h"
#import "Utils.h"

#import <os/log.h>

@interface FlowUDP ()
@property (nonatomic) NEAppProxyUDPFlow *flow;
@property (nonatomic) nw_interface_t interface;
@property (nonatomic) NSMutableDictionary<NSString *, nw_connection_t> *connections;
@property (nonatomic) BOOL closed;
@end

static NSMutableSet<FlowUDP *> *UDPSessions(void)
{
    static NSMutableSet<FlowUDP *> *sessions;
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        sessions = [NSMutableSet set];
    });
    return sessions;
}

@implementation FlowUDP

+ (void)handleFlow:(NEAppProxyUDPFlow *)flow interface:(nw_interface_t)interface
{
    FlowUDP *session = [[FlowUDP alloc] init];
    session.flow = flow;
    session.interface = interface;
    session.connections = [NSMutableDictionary dictionary];
    [UDPSessions() addObject:session];
    [session start];
}

- (void)start
{
    __weak FlowUDP *weakSelf = self;
    void (^opened)(NSError *) = ^(NSError *error) {
        FlowUDP *strongSelf = weakSelf;
        if (strongSelf == nil) {
            return;
        }
        if (error != nil) {
            os_log_error(STUtils.log, "udp: open flow failed: %{public}@", error);
            [strongSelf closeWithError:error];
            return;
        }
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

- (void)readDatagrams
{
    __weak FlowUDP *weakSelf = self;
    [self.flow readDatagramsWithCompletionHandler:^(NSArray<NSData *> *datagrams, NSArray<NWEndpoint *> *remoteEndpoints, NSError *error) {
        FlowUDP *strongSelf = weakSelf;
        if (strongSelf == nil || strongSelf.closed) {
            return;
        }
        if (error != nil) {
            [strongSelf closeWithError:error];
            return;
        }
        if (datagrams.count == 0) {
            [strongSelf closeWithError:nil];
            return;
        }

        NSUInteger count = MIN(datagrams.count, remoteEndpoints.count);
        for (NSUInteger i = 0; i < count; i++) {
            NWEndpoint *endpoint = remoteEndpoints[i];
            if (![endpoint isKindOfClass:[NWHostEndpoint class]]) {
                continue;
            }
            [strongSelf sendDatagram:datagrams[i] toHost:(NWHostEndpoint *)endpoint];
        }
        [strongSelf readDatagrams];
    }];
}

- (void)sendDatagram:(NSData *)datagram toHost:(NWHostEndpoint *)host
{
    NSString *key = [self keyForEndpoint:host];
    nw_connection_t connection = self.connections[key];
    if (connection == NULL) {
        nw_endpoint_t remote = [STUtils copyEndpointFromHost:host.hostname port:host.port];
        if (remote == NULL) {
            return;
        }
        nw_parameters_t params = nw_parameters_create_secure_udp(NW_PARAMETERS_DISABLE_PROTOCOL, NW_PARAMETERS_DEFAULT_CONFIGURATION);
        if (self.interface != NULL) {
            nw_parameters_require_interface(params, self.interface);
        }
        connection = nw_connection_create(remote, params);
        self.connections[key] = connection;
        nw_connection_set_queue(connection, dispatch_get_main_queue());

        __weak FlowUDP *weakSelf = self;
        nw_connection_set_state_changed_handler(connection, ^(nw_connection_state_t state, nw_error_t error) {
            FlowUDP *strongSelf = weakSelf;
            if (strongSelf == nil) {
                return;
            }
            if (state == nw_connection_state_ready) {
                [strongSelf receiveFromConnection:connection host:host];
            } else if (state == nw_connection_state_failed || state == nw_connection_state_cancelled) {
                (void)error;
                [strongSelf.connections removeObjectForKey:key];
            }
        });
        nw_connection_start(connection);
    }

    dispatch_data_t payload = dispatch_data_create(datagram.bytes, datagram.length, dispatch_get_main_queue(), DISPATCH_DATA_DESTRUCTOR_DEFAULT);
    nw_connection_send(connection, payload, NW_CONNECTION_DEFAULT_MESSAGE_CONTEXT, true, ^(nw_error_t sendError) {
        (void)sendError;
    });
}

- (void)receiveFromConnection:(nw_connection_t)connection host:(NWHostEndpoint *)host
{
    __weak FlowUDP *weakSelf = self;
    nw_connection_receive(connection, 1, UINT32_MAX,
                          ^(dispatch_data_t content, nw_content_context_t context, bool isComplete, nw_error_t error) {
                              (void)context;
                              FlowUDP *strongSelf = weakSelf;
                              if (strongSelf == nil || strongSelf.closed) {
                                  return;
                              }
                              if (error != NULL) {
                                  return;
                              }
                              if (content != NULL) {
                                  NSData *data = [STUtils dataFromDispatchData:content];
                                  [strongSelf.flow writeDatagrams:@[ data ]
                                                 sentByEndpoints:@[ host ]
                                               completionHandler:^(NSError *writeError) {
                                                   (void)writeError;
                                               }];
                              }
                              if (!isComplete) {
                                  [strongSelf receiveFromConnection:connection host:host];
                              }
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
    for (nw_connection_t connection in self.connections.allValues) {
        nw_connection_cancel(connection);
    }
    [self.connections removeAllObjects];
    [UDPSessions() removeObject:self];
}

@end
