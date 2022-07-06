#import "./ssconnectivity.h"
#include <arpa/inet.h>
//#import <ShadowSocks/shadowsocks.h>

@import CocoaAsyncSocket;
//@import CocoaLumberjack;

static char *const kShadowsocksLocalAddress = "127.0.0.1";

static char *const kDnsResolverAddress = "208.67.222.222";  // OpenDNS
static const uint16_t kDnsResolverPort = 53;
static const size_t kDnsRequestNumBytes = 28;

static const size_t kSocksHeaderNumBytes = 10;
static const uint8_t kSocksMethodsResponseNumBytes = 2;
static const size_t kSocksConnectResponseNumBytes = 10;
static const uint8_t kSocksVersion = 0x5;
static const uint8_t kSocksMethodNoAuth = 0x0;
static const uint8_t kSocksCmdConnect = 0x1;
static const uint8_t kSocksAtypIpv4 = 0x1;
static const uint8_t kSocksAtypDomainname = 0x3;

static const NSTimeInterval kTcpSocketTimeoutSecs = 10.0;
static const NSTimeInterval kUdpSocketTimeoutSecs = 1.0;
static const long kSocketTagHttpRequest = 100;
static const int kUdpForwardingMaxChecks = 5;
static const uint16_t kHttpPort = 80;

@interface ShadowsocksConnectivity ()<GCDAsyncSocketDelegate, GCDAsyncUdpSocketDelegate>

@property(nonatomic) uint16_t shadowsocksPort;

@property(nonatomic, copy) void (^udpForwardingCompletion)(BOOL);
@property(nonatomic, copy) void (^reachabilityCompletion)(BOOL);
@property(nonatomic, copy) void (^credentialsCompletion)(BOOL);

@property(nonatomic) dispatch_queue_t dispatchQueue;
@property(nonatomic) GCDAsyncUdpSocket *udpSocket;
@property(nonatomic) GCDAsyncSocket *credentialsSocket;
@property(nonatomic) GCDAsyncSocket *reachabilitySocket;

@property(nonatomic) bool isRemoteUdpForwardingEnabled;
@property(nonatomic) bool areServerCredentialsValid;
@property(nonatomic) bool isServerReachable;
@property(nonatomic) int udpForwardingNumChecks;
@end

@implementation ShadowsocksConnectivity

- (id)initWithPort:(uint16_t)shadowsocksPort {
  self = [super init];
  if (self) {
    _shadowsocksPort = shadowsocksPort;
    _dispatchQueue = dispatch_queue_create("ShadowsocksConnectivity", DISPATCH_QUEUE_SERIAL);
  }
  return self;
}

#pragma mark - UDP Forwarding

struct socks_udp_header {
  uint16_t rsv;
  uint8_t frag;
  uint8_t atyp;
  uint32_t addr;
  uint16_t port;
};

- (void)isUdpForwardingEnabled:(void (^)(BOOL))completion {
//  DDLogInfo(@"Starting remote UDP forwarding check.");
  self.isRemoteUdpForwardingEnabled = false;
  self.udpForwardingNumChecks = 0;
  self.udpForwardingCompletion = completion;
  self.udpSocket =
      [[GCDAsyncUdpSocket alloc] initWithDelegate:self delegateQueue:self.dispatchQueue];
  struct in_addr dnsResolverAddress;
  if (!inet_aton(kDnsResolverAddress, &dnsResolverAddress)) {
//    DDLogError(@"Failed to convert DNS resolver IP.");
    [self udpForwardingCheckDone:false];
    return;
  }
  struct socks_udp_header socksHeader = {
      .atyp = kSocksAtypIpv4,
      .addr = dnsResolverAddress.s_addr,  // Already in network order
      .port = htons(kDnsResolverPort)};
  uint8_t *dnsRequest = [self getDnsRequest];
  size_t packetNumBytes = kSocksHeaderNumBytes + kDnsRequestNumBytes;
  uint8_t socksPacket[packetNumBytes];
  memset(socksPacket, 0, packetNumBytes);
  memcpy(socksPacket, &socksHeader, kSocksHeaderNumBytes);
  memcpy(socksPacket + kSocksHeaderNumBytes, dnsRequest, kDnsRequestNumBytes);

  NSData *packetData = [[NSData alloc] initWithBytes:socksPacket length:packetNumBytes];

  dispatch_source_t timer =
      dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, self.dispatchQueue);
  if (!timer) {
//    DDLogError(@"Failed to create timer");
    [self udpForwardingCheckDone:false];
    return;
  }
  dispatch_source_set_timer(timer, dispatch_time(DISPATCH_TIME_NOW, 0),
                            kUdpSocketTimeoutSecs * NSEC_PER_SEC, 0);
  __weak ShadowsocksConnectivity *weakSelf = self;
  dispatch_source_set_event_handler(timer, ^{
    if (++weakSelf.udpForwardingNumChecks > kUdpForwardingMaxChecks ||
        weakSelf.isRemoteUdpForwardingEnabled) {
      dispatch_source_cancel(timer);
      if (!weakSelf.isRemoteUdpForwardingEnabled) {
        [weakSelf udpForwardingCheckDone:false];
      }
      [weakSelf.udpSocket close];
      return;
    }
//    DDLogDebug(@"Checking remote server's UDP forwarding (%d of %d).",
//               weakSelf.udpForwardingNumChecks, kUdpForwardingMaxChecks);
    [weakSelf.udpSocket sendData:packetData
                          toHost:[[NSString alloc] initWithUTF8String:kShadowsocksLocalAddress]
                            port:self.shadowsocksPort
                     withTimeout:kUdpSocketTimeoutSecs
                             tag:0];
    if (![weakSelf.udpSocket receiveOnce:nil]) {
//      DDLogError(@"UDP socket failed to receive data");
    }
  });
  dispatch_resume(timer);
}

// Returns a byte representation of a DNS request for "google.com".
- (uint8_t *)getDnsRequest {
  static uint8_t kDnsRequest[] = {
      0, 0,  // [0-1]   query ID
      1, 0,  // [2-3]   flags; byte[2] = 1 for recursion desired (RD).
      0, 1,  // [4-5]   QDCOUNT (number of queries)
      0, 0,  // [6-7]   ANCOUNT (number of answers)
      0, 0,  // [8-9]   NSCOUNT (number of name server records)
      0, 0,  // [10-11] ARCOUNT (number of additional records)
      6, 'g', 'o', 'o', 'g', 'l', 'e', 3, 'c', 'o', 'm',
      0,     // null terminator of FQDN (root TLD)
      0, 1,  // QTYPE, set to A
      0, 1   // QCLASS, set to 1 = IN (Internet)
  };
  return kDnsRequest;
}

#pragma mark - GCDAsyncUdpSocketDelegate

- (void)udpSocket:(GCDAsyncUdpSocket *)sock
    didNotSendDataWithTag:(long)tag
               dueToError:(NSError *)error {
//  DDLogError(@"Failed to send data on UDP socket");
}

- (void)udpSocket:(GCDAsyncUdpSocket *)sock
       didReceiveData:(NSData *)data
          fromAddress:(NSData *)address
    withFilterContext:(id)filterContext {
  if (!self.isRemoteUdpForwardingEnabled) {
    // Only report success if it hasn't been done so already.
    [self udpForwardingCheckDone:true];
  }
}

- (void)udpForwardingCheckDone:(BOOL)enabled {
//  DDLogInfo(@"Remote UDP forwarding %@", enabled ? @"enabled" : @"disabled");
  self.isRemoteUdpForwardingEnabled = enabled;
  if (self.udpForwardingCompletion != NULL) {
    self.udpForwardingCompletion(self.isRemoteUdpForwardingEnabled);
    self.udpForwardingCompletion = NULL;
  }
}

#pragma mark - Credentials

struct socks_methods_request {
  uint8_t ver;
  uint8_t nmethods;
  uint8_t method;
};

struct socks_request_header {
  uint8_t ver;
  uint8_t cmd;
  uint8_t rsv;
  uint8_t atyp;
};

- (void)checkServerCredentials:(void (^)(BOOL))completion {
//  DDLogInfo(@"Starting server creds. validation");
  self.areServerCredentialsValid = false;
  self.credentialsCompletion = completion;
  self.credentialsSocket =
      [[GCDAsyncSocket alloc] initWithDelegate:self delegateQueue:self.dispatchQueue];
  NSError *error;
  [self.credentialsSocket
      connectToHost:[[NSString alloc] initWithUTF8String:kShadowsocksLocalAddress]
             onPort:self.shadowsocksPort
        withTimeout:kTcpSocketTimeoutSecs
              error:&error];
  if (error) {
//    DDLogError(@"Unable to connect to local Shadowsocks server.");
    [self serverCredentialsCheckDone];
    return;
  }

  struct socks_methods_request methodsRequest = {
      .ver = kSocksVersion, .nmethods = 0x1, .method = kSocksMethodNoAuth};
  NSData *methodsRequestData =
      [[NSData alloc] initWithBytes:&methodsRequest length:sizeof(struct socks_methods_request)];
  [self.credentialsSocket writeData:methodsRequestData withTimeout:kTcpSocketTimeoutSecs tag:0];
  [self.credentialsSocket readDataToLength:kSocksMethodsResponseNumBytes
                               withTimeout:kTcpSocketTimeoutSecs
                                       tag:0];

  size_t socksRequestHeaderNumBytes = sizeof(struct socks_request_header);
  NSString *domain = [self chooseRandomDomain];
  uint8_t domainNameNumBytes = domain.length;
  size_t socksRequestNumBytes = socksRequestHeaderNumBytes + domainNameNumBytes +
                                sizeof(uint16_t) /* port */ +
                                sizeof(uint8_t) /* domain name length */;

  struct socks_request_header socksRequestHeader = {
      .ver = kSocksVersion, .cmd = kSocksCmdConnect, .atyp = kSocksAtypDomainname};
  uint8_t socksRequest[socksRequestNumBytes];
  memset(socksRequest, 0x0, socksRequestNumBytes);
  memcpy(socksRequest, &socksRequestHeader, socksRequestHeaderNumBytes);
  socksRequest[socksRequestHeaderNumBytes] = domainNameNumBytes;
  memcpy(socksRequest + socksRequestHeaderNumBytes + sizeof(uint8_t), [domain UTF8String],
         domainNameNumBytes);
  uint16_t httpPort = htons(kHttpPort);
  memcpy(socksRequest + socksRequestHeaderNumBytes + sizeof(uint8_t) + domainNameNumBytes,
         &httpPort, sizeof(uint16_t));

  NSData *socksRequestData =
      [[NSData alloc] initWithBytes:socksRequest length:socksRequestNumBytes];
  [self.credentialsSocket writeData:socksRequestData withTimeout:kTcpSocketTimeoutSecs tag:0];
  [self.credentialsSocket readDataToLength:kSocksConnectResponseNumBytes
                               withTimeout:kTcpSocketTimeoutSecs
                                       tag:0];

  NSString *httpRequest =
      [[NSString alloc] initWithFormat:@"HEAD / HTTP/1.1\r\nHost: %@\r\n\r\n", domain];
  [self.credentialsSocket
        writeData:[NSData dataWithBytes:[httpRequest UTF8String] length:httpRequest.length]
      withTimeout:kTcpSocketTimeoutSecs
              tag:kSocketTagHttpRequest];
  [self.credentialsSocket readDataWithTimeout:kTcpSocketTimeoutSecs tag:kSocketTagHttpRequest];
  [self.credentialsSocket disconnectAfterReading];
}

// Returns a statically defined array containing domain names for validating server credentials.
+ (const NSArray *)getCredentialsValidationDomains {
  static const NSArray *kCredentialsValidationDomains;
  static dispatch_once_t kDispatchOnceToken;
  dispatch_once(&kDispatchOnceToken, ^{
    // We have chosen these domains due to their neutrality.
    kCredentialsValidationDomains =
        @[ @"eff.org", @"ietf.org", @"w3.org", @"wikipedia.org", @"example.com" ];
  });
  return kCredentialsValidationDomains;
}

// Returns a random domain from |kCredentialsValidationDomains|.
- (NSString *)chooseRandomDomain {
  const NSArray *domains = [ShadowsocksConnectivity getCredentialsValidationDomains];
  int index = arc4random_uniform((uint32_t)domains.count);
  return domains[index];
}

// Calls |credentialsCompletion| once with |areServerCredentialsValid|.
- (void)serverCredentialsCheckDone {
//  DDLogInfo(@"Server creds. %@.", self.areServerCredentialsValid ? @"succeeded" : @"failed");
  if (self.credentialsCompletion != NULL) {
    self.credentialsCompletion(self.areServerCredentialsValid);
    self.credentialsCompletion = NULL;
  }
}

#pragma mark - Reachability

- (void)isReachable:(NSString *)host port:(uint16_t)port completion:(void (^)(BOOL))completion {
//  DDLogInfo(@"Starting server reachability check.");
  self.isServerReachable = false;
  self.reachabilityCompletion = completion;
  self.reachabilitySocket =
      [[GCDAsyncSocket alloc] initWithDelegate:self delegateQueue:self.dispatchQueue];
  NSError *error;
  [self.reachabilitySocket connectToHost:host
                                  onPort:port
                             withTimeout:kTcpSocketTimeoutSecs
                                   error:&error];
  if (error) {
//    DDLogError(@"Unable to connect to Shadowsocks server.");
    return;
  }
}

// Calls |reachabilityCompletion| once with |isServerReachable|.
- (void)reachabilityCheckDone {
//  DDLogInfo(@"Server %@.", self.isServerReachable ? @"reachable" : @"unreachable");
  if (self.reachabilityCompletion != NULL) {
    self.reachabilityCompletion(self.isServerReachable);
    self.reachabilityCompletion = NULL;
  }
}

#pragma mark - GCDAsyncSocketDelegate

- (void)socket:(GCDAsyncSocket *)sock didReadData:(NSData *)data withTag:(long)tag {
  // We don't need to inspect any of the data, as the SOCKS responses are hardcoded in ss-local and
  // the fact that we have read the HTTP response indicates that the server credentials are valid.
  if (tag == kSocketTagHttpRequest && data != nil) {
    NSString *httpResponse = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
    self.areServerCredentialsValid = httpResponse != nil && [httpResponse hasPrefix:@"HTTP/1.1"];
  }
}

- (void)socket:(GCDAsyncSocket *)sock didConnectToHost:(NSString *)host port:(uint16_t)port {
  if ([self.reachabilitySocket isEqual:sock]) {
    self.isServerReachable = true;
    [self.reachabilitySocket disconnect];
  }
}

- (void)socketDidDisconnect:(GCDAsyncSocket *)sock withError:(NSError *)error {
  if ([self.reachabilitySocket isEqual:sock]) {
    [self reachabilityCheckDone];
  } else {
    [self serverCredentialsCheckDone];
  }
}

@end

