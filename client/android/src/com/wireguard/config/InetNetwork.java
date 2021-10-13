/*
 * Copyright © 2017-2019 WireGuard LLC. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.wireguard.config;

import java.net.Inet4Address;
import java.net.InetAddress;

/**
 * An Internet network, denoted by its address and netmask
 * <p>
 * Instances of this class are immutable.
 */

public final class InetNetwork {
  private final InetAddress address;
  private final int mask;

  private InetNetwork(final InetAddress address, final int mask) {
    this.address = address;
    this.mask = mask;
  }

  public static InetNetwork parse(final String network) throws ParseException {
    final int slash = network.lastIndexOf('/');
    final String maskString;
    final int rawMask;
    final String rawAddress;
    if (slash >= 0) {
      maskString = network.substring(slash + 1);
      try {
        rawMask = Integer.parseInt(maskString, 10);
      } catch (final NumberFormatException ignored) {
        throw new ParseException(Integer.class, maskString);
      }
      rawAddress = network.substring(0, slash);
    } else {
      maskString = "";
      rawMask = -1;
      rawAddress = network;
    }
    final InetAddress address = InetAddresses.parse(rawAddress);
    final int maxMask = (address instanceof Inet4Address) ? 32 : 128;
    if (rawMask > maxMask)
      throw new ParseException(InetNetwork.class, maskString, "Invalid network mask");
    final int mask = rawMask >= 0 ? rawMask : maxMask;
    return new InetNetwork(address, mask);
  }

  @Override
  public boolean equals(final Object obj) {
    if (!(obj instanceof InetNetwork))
      return false;
    final InetNetwork other = (InetNetwork) obj;
    return address.equals(other.address) && mask == other.mask;
  }

  public InetAddress getAddress() {
    return address;
  }

  public int getMask() {
    return mask;
  }

  @Override
  public int hashCode() {
    return address.hashCode() ^ mask;
  }

  @Override
  public String toString() {
    return address.getHostAddress() + '/' + mask;
  }
}
