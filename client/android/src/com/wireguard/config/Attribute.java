/*
 * Copyright © 2018-2019 WireGuard LLC. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.wireguard.config;

import java.util.Iterator;
import java.util.Optional;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public final class Attribute {
  private static final Pattern LINE_PATTERN = Pattern.compile("(\\w+)\\s*=\\s*([^\\s#][^#]*)");
  private static final Pattern LIST_SEPARATOR = Pattern.compile("\\s*,\\s*");

  private final String key;
  private final String value;

  private Attribute(final String key, final String value) {
    this.key = key;
    this.value = value;
  }

  public static String join(final Iterable<?> values) {
    final Iterator<?> it = values.iterator();
    if (!it.hasNext()) {
      return "";
    }
    final StringBuilder sb = new StringBuilder();
    sb.append(it.next());
    while (it.hasNext()) {
      sb.append(", ");
      sb.append(it.next());
    }
    return sb.toString();
  }

  public static Optional<Attribute> parse(final CharSequence line) {
    final Matcher matcher = LINE_PATTERN.matcher(line);
    if (!matcher.matches())
      return Optional.empty();
    return Optional.of(new Attribute(matcher.group(1), matcher.group(2)));
  }

  public static String[] split(final CharSequence value) {
    return LIST_SEPARATOR.split(value);
  }

  public String getKey() {
    return key;
  }

  public String getValue() {
    return value;
  }
}
