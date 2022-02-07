/*
 * Copyright © 2017-2019 WireGuard LLC. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.wireguard.config;

import java.lang.reflect.Method;
import java.net.Inet4Address;
import java.net.Inet6Address;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.regex.Pattern;

import androidx.annotation.Nullable;

/**
 * Utility methods for creating instances of {@link InetAddress}.
 */

public final class InetAddresses {
  @Nullable private static final Method PARSER_METHOD;
  private static final Pattern WONT_TOUCH_RESOLVER = Pattern.compile(
      "^(((([0-9A-Fa-f]{1,4}:){7}([0-9A-Fa-f]{1,4}|:))|(([0-9A-Fa-f]{1,4}:){6}(:[0-9A-Fa-f]{1,4}|((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3})|:))|(([0-9A-Fa-f]{1,4}:){5}(((:[0-9A-Fa-f]{1,4}){1,2})|:((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3})|:))|(([0-9A-Fa-f]{1,4}:){4}(((:[0-9A-Fa-f]{1,4}){1,3})|((:[0-9A-Fa-f]{1,4})?:((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){3}(((:[0-9A-Fa-f]{1,4}){1,4})|((:[0-9A-Fa-f]{1,4}){0,2}:((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){2}(((:[0-9A-Fa-f]{1,4}){1,5})|((:[0-9A-Fa-f]{1,4}){0,3}:((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){1}(((:[0-9A-Fa-f]{1,4}){1,6})|((:[0-9A-Fa-f]{1,4}){0,4}:((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3}))|:))|(:(((:[0-9A-Fa-f]{1,4}){1,7})|((:[0-9A-Fa-f]{1,4}){0,5}:((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3}))|:)))(%.+)?)|((?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?))$");

  static {
    Method m = null;
    try {
      if (android.os.Build.VERSION.SDK_INT < android.os.Build.VERSION_CODES.Q)
        // noinspection JavaReflectionMemberAccess
        m = InetAddress.class.getMethod("parseNumericAddress", String.class);
    } catch (final Exception ignored) {
    }
    PARSER_METHOD = m;
  }

  private InetAddresses() {}

  /**
   * Parses a numeric IPv4 or IPv6 address without performing any DNS lookups.
   *
   * @param address a string representing the IP address
   * @return an instance of {@link Inet4Address} or {@link Inet6Address}, as appropriate
   */
  public static InetAddress parse(final String address) throws ParseException {
    if (address.isEmpty())
      throw new ParseException(InetAddress.class, address, "Empty address");
    try {
      if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.Q)
        return android.net.InetAddresses.parseNumericAddress(address);
      else if (PARSER_METHOD != null)
        return (InetAddress) PARSER_METHOD.invoke(null, address);
      else
        throw new NoSuchMethodException("parseNumericAddress");
    } catch (final IllegalArgumentException e) {
      throw new ParseException(InetAddress.class, address, e);
    } catch (final Exception e) {
      final Throwable cause = e.getCause();
      // Re-throw parsing exceptions with the original type, as callers might try to catch
      // them. On the other hand, callers cannot be expected to handle reflection failures.
      if (cause instanceof IllegalArgumentException)
        throw new ParseException(InetAddress.class, address, cause);
      try {
        if (WONT_TOUCH_RESOLVER.matcher(address).matches())
          return InetAddress.getByName(address);
        else
          throw new ParseException(InetAddress.class, address, "Not an IP address");
      } catch (final UnknownHostException f) {
        throw new ParseException(InetAddress.class, address, f);
      }
    }
  }
}
