// Copyright 2018 The Outline Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef ShadowsocksConnectivity_h
#define ShadowsocksConnectivity_h

#import <Foundation/Foundation.h>

/**
 * Non-thread-safe class to perform Shadowsocks connectivity checks.
 */
@interface ShadowsocksConnectivity : NSObject

/**
 * Initializes the object with a local Shadowsocks port, |shadowsocksPort|.
 */
- (id)initWithPort:(uint16_t)shadowsocksPort;

/**
 * Verifies that the server has enabled UDP forwarding. Performs an end-to-end test by sending
 * a DNS request through the proxy. This method is a superset of |checkServerCredentials|, as its
 * success implies that the server credentials are valid.
 */
- (void)isUdpForwardingEnabled:(void (^)(BOOL))completion;

/**
 * Verifies that the server credentials are valid. Performs an end-to-end authentication test
 * by issuing an HTTP HEAD request to a target domain through the proxy.
 */
- (void)checkServerCredentials:(void (^)(BOOL))completion;

/**
 * Checks that the server is reachable on |host| and |port|.
 */
- (void)isReachable:(NSString *)host port:(uint16_t)port completion:(void (^)(BOOL))completion;

@end

#endif /* ShadowsocksConnectivity_h */
