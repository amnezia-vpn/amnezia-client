/*
 * Copyright © 2018-2019 WireGuard LLC. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.wireguard.config;

import com.wireguard.crypto.KeyFormatException;

import androidx.annotation.Nullable;

public class BadConfigException extends Exception {
  private final Location location;
  private final Reason reason;
  private final Section section;
  @Nullable private final CharSequence text;

  private BadConfigException(final Section section, final Location location, final Reason reason,
      @Nullable final CharSequence text, @Nullable final Throwable cause) {
    super(cause);
    this.section = section;
    this.location = location;
    this.reason = reason;
    this.text = text;
  }

  public BadConfigException(final Section section, final Location location, final Reason reason,
      @Nullable final CharSequence text) {
    this(section, location, reason, text, null);
  }

  public BadConfigException(
      final Section section, final Location location, final KeyFormatException cause) {
    this(section, location, Reason.INVALID_KEY, null, cause);
  }

  public BadConfigException(final Section section, final Location location,
      @Nullable final CharSequence text, final NumberFormatException cause) {
    this(section, location, Reason.INVALID_NUMBER, text, cause);
  }

  public BadConfigException(
      final Section section, final Location location, final ParseException cause) {
    this(section, location, Reason.INVALID_VALUE, cause.getText(), cause);
  }

  public Location getLocation() {
    return location;
  }

  public Reason getReason() {
    return reason;
  }

  public Section getSection() {
    return section;
  }

  @Nullable
  public CharSequence getText() {
    return text;
  }

  public enum Location {
    TOP_LEVEL(""),
    ADDRESS("Address"),
    ALLOWED_IPS("AllowedIPs"),
    DNS("DNS"),
    ENDPOINT("Endpoint"),
    EXCLUDED_APPLICATIONS("ExcludedApplications"),
    INCLUDED_APPLICATIONS("IncludedApplications"),
    LISTEN_PORT("ListenPort"),
    MTU("MTU"),
    PERSISTENT_KEEPALIVE("PersistentKeepalive"),
    PRE_SHARED_KEY("PresharedKey"),
    PRIVATE_KEY("PrivateKey"),
    PUBLIC_KEY("PublicKey");

    private final String name;

    Location(final String name) {
      this.name = name;
    }

    public String getName() {
      return name;
    }
  }

  public enum Reason {
    INVALID_KEY,
    INVALID_NUMBER,
    INVALID_VALUE,
    MISSING_ATTRIBUTE,
    MISSING_SECTION,
    SYNTAX_ERROR,
    UNKNOWN_ATTRIBUTE,
    UNKNOWN_SECTION
  }

  public enum Section {
    CONFIG("Config"),
    INTERFACE("Interface"),
    PEER("Peer");

    private final String name;

    Section(final String name) {
      this.name = name;
    }

    public String getName() {
      return name;
    }
  }
}
