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

import Foundation

// Represents an IP subnetwork.
@objcMembers
class Subnet: NSObject {
  static let kReservedSubnets = [
    "10.0.0.0/8",
    "100.64.0.0/10",
    "169.254.0.0/16",
    "172.16.0.0/12",
    "192.0.0.0/24",
    "192.0.2.0/24",
    "192.31.196.0/24",
    "192.52.193.0/24",
    "192.88.99.0/24",
    "192.168.0.0/16",
    "192.175.48.0/24",
    "198.18.0.0/15",
    "198.51.100.0/24",
    "203.0.113.0/24",
    "240.0.0.0/4"
  ]

  // Parses a CIDR subnet into a Subnet object. Returns nil on failure.
  static func parse(_ cidrSubnet: String) -> Subnet? {
    let components = cidrSubnet.components(separatedBy: "/")
    guard components.count == 2 else {
      NSLog("Malformed CIDR subnet")
      return nil
    }
    guard let prefix = UInt16(components[1]) else {
      NSLog("Invalid subnet prefix")
      return nil
    }
    return Subnet(address: components[0], prefix: prefix)
  }

  // Returns a list of reserved Subnets.
  static func getReservedSubnets() -> [Subnet] {
    var subnets: [Subnet] = []
    for cidrSubnet in kReservedSubnets {
      if let subnet = self.parse(cidrSubnet) {
        subnets.append(subnet)
      }
    }
    return subnets
  }

  public var address: String
  public var prefix: UInt16
  public var mask: String

  init(address: String, prefix: UInt16) {
    self.address = address
    self.prefix = prefix
    let mask = (0xffffffff as UInt32) << (32 - prefix);
    self.mask = mask.IPv4String()
  }
}

extension UInt32 {
  // Returns string representation of the integer as an IP address.
  public func IPv4String() -> String {
    let ip = self
    let a = UInt8((ip>>24) & 0xff)
    let b = UInt8((ip>>16) & 0xff)
    let c = UInt8((ip>>8) & 0xff)
    let d = UInt8(ip & 0xff)
    return "\(a).\(b).\(c).\(d)"
  }
}

