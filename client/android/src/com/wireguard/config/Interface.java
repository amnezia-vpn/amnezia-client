/*
 * Copyright © 2017-2019 WireGuard LLC. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.wireguard.config;

import com.wireguard.config.BadConfigException.Location;
import com.wireguard.config.BadConfigException.Reason;
import com.wireguard.config.BadConfigException.Section;
import com.wireguard.crypto.Key;
import com.wireguard.crypto.KeyFormatException;
import com.wireguard.crypto.KeyPair;

import java.net.InetAddress;
import java.util.Collection;
import java.util.Collections;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Locale;
import java.util.Objects;
import java.util.Optional;
import java.util.Set;
import java.util.stream.Collectors;

import androidx.annotation.Nullable;

/**
 * Represents the configuration for a WireGuard interface (an [Interface] block). Interfaces must
 * have a private key (used to initialize a {@code KeyPair}), and may optionally have several other
 * attributes.
 * <p>
 * Instances of this class are immutable.
 */

public final class Interface {
  private static final int MAX_UDP_PORT = 65535;
  private static final int MIN_UDP_PORT = 0;

  private final Set<InetNetwork> addresses;
  private final Set<InetAddress> dnsServers;
  private final Set<String> excludedApplications;
  private final Set<String> includedApplications;
  private final KeyPair keyPair;
  private final Optional<Integer> listenPort;
  private final Optional<Integer> mtu;
  private final Optional<Integer> jc;
  private final Optional<Integer> jmin;
  private final Optional<Integer> jmax;
  private final Optional<Integer> s1;
  private final Optional<Integer> s2;
  private final Optional<Long> h1;
  private final Optional<Long> h2;
  private final Optional<Long> h3;
  private final Optional<Long> h4;

  private Interface(final Builder builder) {
    // Defensively copy to ensure immutability even if the Builder is reused.
    addresses = Collections.unmodifiableSet(new LinkedHashSet<>(builder.addresses));
    dnsServers = Collections.unmodifiableSet(new LinkedHashSet<>(builder.dnsServers));
    excludedApplications =
        Collections.unmodifiableSet(new LinkedHashSet<>(builder.excludedApplications));
    includedApplications =
        Collections.unmodifiableSet(new LinkedHashSet<>(builder.includedApplications));
    keyPair = Objects.requireNonNull(builder.keyPair, "Interfaces must have a private key");
    listenPort = builder.listenPort;
    mtu = builder.mtu;
    jc = builder.jc;
    jmax = builder.jmax;
    jmin = builder.jmin;
    s1 = builder.s1;
    s2 = builder.s2;
    h1 = builder.h1;
    h2 = builder.h2;
    h3 = builder.h3;
    h4 = builder.h4;
  }

  /**
   * Parses an series of "KEY = VALUE" lines into an {@code Interface}. Throws
   * {@link ParseException} if the input is not well-formed or contains unknown attributes.
   *
   * @param lines An iterable sequence of lines, containing at least a private key attribute
   * @return An {@code Interface} with all of the attributes from {@code lines} set
   */
  public static Interface parse(final Iterable<? extends CharSequence> lines)
      throws BadConfigException {
    final Builder builder = new Builder();
    for (final CharSequence line : lines) {
      final Attribute attribute =
          Attribute.parse(line).orElseThrow(()
                                                -> new BadConfigException(Section.INTERFACE,
                                                    Location.TOP_LEVEL, Reason.SYNTAX_ERROR, line));
      switch (attribute.getKey().toLowerCase(Locale.ENGLISH)) {
        case "address":
          builder.parseAddresses(attribute.getValue());
          break;
        case "dns":
          builder.parseDnsServers(attribute.getValue());
          break;
        case "excludedapplications":
          builder.parseExcludedApplications(attribute.getValue());
          break;
        case "includedapplications":
          builder.parseIncludedApplications(attribute.getValue());
          break;
        case "listenport":
          builder.parseListenPort(attribute.getValue());
          break;
        case "mtu":
          builder.parseMtu(attribute.getValue());
          break;
        case "privatekey":
          builder.parsePrivateKey(attribute.getValue());
          break;
        case "jc":
          builder.parseJc(attribute.getValue());
          break;
        case "jmin":
          builder.parseJmin(attribute.getValue());
          break;
        case "jmax":
          builder.parseJmax(attribute.getValue());
          break;
        case "s1":
          builder.parseS1(attribute.getValue());
          break;                   
        case "s2":
          builder.parseS2(attribute.getValue());
          break;
        case "h1":
          builder.parseH1(attribute.getValue());
          break;          
        case "h2":
          builder.parseH2(attribute.getValue());
          break;
        case "h3":
          builder.parseH3(attribute.getValue());
          break; 
        case "h4":
          builder.parseH4(attribute.getValue());
          break;                          
        default:
          throw new BadConfigException(
              Section.INTERFACE, Location.TOP_LEVEL, Reason.UNKNOWN_ATTRIBUTE, attribute.getKey());
      }
    }
    return builder.build();
  }

  @Override
  public boolean equals(final Object obj) {
    if (!(obj instanceof Interface))
      return false;
    final Interface other = (Interface) obj;
    return addresses.equals(other.addresses) && dnsServers.equals(other.dnsServers)
        && excludedApplications.equals(other.excludedApplications)
        && includedApplications.equals(other.includedApplications) && keyPair.equals(other.keyPair)
        && listenPort.equals(other.listenPort) && mtu.equals(other.mtu) && jc.equals(other.jc) && jmin.equals(other.jmin)
        && jmax.equals(other.jmax) && s1.equals(other.s1) && s2.equals(other.s2) && h1.equals(other.h1) && h2.equals(other.h2)
        && h3.equals(other.h3) && h4.equals(other.h4);
  }

  /**
   * Returns the set of IP addresses assigned to the interface.
   *
   * @return a set of {@link InetNetwork}s
   */
  public Set<InetNetwork> getAddresses() {
    // The collection is already immutable.
    return addresses;
  }

  /**
   * Returns the set of DNS servers associated with the interface.
   *
   * @return a set of {@link InetAddress}es
   */
  public Set<InetAddress> getDnsServers() {
    // The collection is already immutable.
    return dnsServers;
  }

  /**
   * Returns the set of applications excluded from using the interface.
   *
   * @return a set of package names
   */
  public Set<String> getExcludedApplications() {
    // The collection is already immutable.
    return excludedApplications;
  }

  /**
   * Returns the set of applications included exclusively for using the interface.
   *
   * @return a set of package names
   */
  public Set<String> getIncludedApplications() {
    // The collection is already immutable.
    return includedApplications;
  }

  /**
   * Returns the public/private key pair used by the interface.
   *
   * @return a key pair
   */
  public KeyPair getKeyPair() {
    return keyPair;
  }

  /**
   * Returns the UDP port number that the WireGuard interface will listen on.
   *
   * @return a UDP port number, or {@code Optional.empty()} if none is configured
   */
  public Optional<Integer> getListenPort() {
    return listenPort;
  }

  /**
   * Returns the MTU used for the WireGuard interface.
   *
   * @return the MTU, or {@code Optional.empty()} if none is configured
   */
  public Optional<Integer> getMtu() {
    return mtu;
  }
  
  public Optional<Integer> getJc() {
    return jc;
  }
  
  public Optional<Integer> getJmin() {
    return jmin;
  }

  public Optional<Integer> getJmax() {
    return jmax;
  }

  public Optional<Integer> getS1() {
    return s1;
  }

  public Optional<Integer> getS2() {
    return s2;
  }

  public Optional<Long> getH1() {
    return h1;
  }

  public Optional<Long> getH2() {
    return h2;
  }

  public Optional<Long> getH3() {
    return h3;
  }

  public Optional<Long> getH4() {
    return h4;
  }

  @Override
  public int hashCode() {
    int hash = 1;
    hash = 31 * hash + addresses.hashCode();
    hash = 31 * hash + dnsServers.hashCode();
    hash = 31 * hash + excludedApplications.hashCode();
    hash = 31 * hash + includedApplications.hashCode();
    hash = 31 * hash + keyPair.hashCode();
    hash = 31 * hash + listenPort.hashCode();
    hash = 31 * hash + mtu.hashCode();
    hash = 31 * hash + jc.hashCode();
    hash = 31 * hash + jmin.hashCode();
    hash = 31 * hash + jmax.hashCode();
    hash = 31 * hash + s1.hashCode();
    hash = 31 * hash + s2.hashCode();
    hash = 31 * hash + h1.hashCode();
    hash = 31 * hash + h2.hashCode();
    hash = 31 * hash + h3.hashCode();
    hash = 31 * hash + h4.hashCode();
    return hash;
  }

  /**
   * Converts the {@code Interface} into a string suitable for debugging purposes. The {@code
   * Interface} is identified by its public key and (if set) the port used for its UDP socket.
   *
   * @return A concise single-line identifier for the {@code Interface}
   */
  @Override
  public String toString() {
    final StringBuilder sb = new StringBuilder("(Interface ");
    sb.append(keyPair.getPublicKey().toBase64());
    listenPort.ifPresent(lp -> sb.append(" @").append(lp));
    sb.append(')');
    return sb.toString();
  }

  /**
   * Converts the {@code Interface} into a string suitable for inclusion in a {@code wg-quick}
   * configuration file.
   *
   * @return The {@code Interface} represented as a series of "Key = Value" lines
   */
  public String toWgQuickString() {
    final StringBuilder sb = new StringBuilder();
    if (!addresses.isEmpty())
      sb.append("Address = ").append(Attribute.join(addresses)).append('\n');
    if (!dnsServers.isEmpty()) {
      final List<String> dnsServerStrings =
          dnsServers.stream().map(InetAddress::getHostAddress).collect(Collectors.toList());
      sb.append("DNS = ").append(Attribute.join(dnsServerStrings)).append('\n');
    }
    if (!excludedApplications.isEmpty())
      sb.append("ExcludedApplications = ")
          .append(Attribute.join(excludedApplications))
          .append('\n');
    if (!includedApplications.isEmpty())
      sb.append("IncludedApplications = ")
          .append(Attribute.join(includedApplications))
          .append('\n');
    listenPort.ifPresent(lp -> sb.append("ListenPort = ").append(lp).append('\n'));
    mtu.ifPresent(m -> sb.append("MTU = ").append(m).append('\n'));
    
    jc.ifPresent(t_jc -> sb.append("Jc = ").append(t_jc).append('\n'));
    jmin.ifPresent(t_jmin -> sb.append("Jmin = ").append(t_jmin).append('\n'));
    jmax.ifPresent(t_jmax -> sb.append("Jmax = ").append(t_jmax).append('\n'));

    s1.ifPresent(t_s1 -> sb.append("S1 = ").append(t_s1).append('\n'));
    s2.ifPresent(t_s2 -> sb.append("S2 = ").append(t_s2).append('\n'));

    h1.ifPresent(t_h1 -> sb.append("H1 = ").append(t_h1).append('\n'));
    h2.ifPresent(t_h2 -> sb.append("H2 = ").append(t_h2).append('\n'));
    h3.ifPresent(t_h3 -> sb.append("H3 = ").append(t_h3).append('\n'));
    h4.ifPresent(t_h4 -> sb.append("H4 = ").append(t_h4).append('\n'));
    
    sb.append("PrivateKey = ").append(keyPair.getPrivateKey().toBase64()).append('\n');
    return sb.toString();
  }

  /**
   * Serializes the {@code Interface} for use with the WireGuard cross-platform userspace API.
   * Note that not all attributes are included in this representation.
   *
   * @return the {@code Interface} represented as a series of "KEY=VALUE" lines
   */
  public String toWgUserspaceString() {
    final StringBuilder sb = new StringBuilder();
    sb.append("private_key=").append(keyPair.getPrivateKey().toHex()).append('\n');
    listenPort.ifPresent(lp -> sb.append("listen_port=").append(lp).append('\n'));

    jc.ifPresent(t_jc -> sb.append("jc=").append(t_jc).append('\n'));
    jmin.ifPresent(t_jmin -> sb.append("jmin=").append(t_jmin).append('\n'));
    jmax.ifPresent(t_jmax -> sb.append("jmax=").append(t_jmax).append('\n'));
    
    s1.ifPresent(t_s1 -> sb.append("s1=").append(t_s1).append('\n'));
    s2.ifPresent(t_s2 -> sb.append("s2=").append(t_s2).append('\n'));
    
    h1.ifPresent(t_h1 -> sb.append("h1=").append(t_h1).append('\n'));
    h2.ifPresent(t_h2 -> sb.append("h2=").append(t_h2).append('\n'));
    h3.ifPresent(t_h3 -> sb.append("h3=").append(t_h3).append('\n'));
    h4.ifPresent(t_h4 -> sb.append("h4=").append(t_h4).append('\n'));    
    return sb.toString();
  }

  @SuppressWarnings("UnusedReturnValue")
  public static final class Builder {
    // Defaults to an empty set.
    private final Set<InetNetwork> addresses = new LinkedHashSet<>();
    // Defaults to an empty set.
    private final Set<InetAddress> dnsServers = new LinkedHashSet<>();
    // Defaults to an empty set.
    private final Set<String> excludedApplications = new LinkedHashSet<>();
    // Defaults to an empty set.
    private final Set<String> includedApplications = new LinkedHashSet<>();
    // No default; must be provided before building.
    @Nullable private KeyPair keyPair;
    // Defaults to not present.
    private Optional<Integer> listenPort = Optional.empty();
    // Defaults to not present.
    private Optional<Integer> mtu = Optional.empty();
    private Optional<Integer> jc = Optional.empty();
    private Optional<Integer> jmin = Optional.empty();
    private Optional<Integer> jmax = Optional.empty();
    
    private Optional<Integer> s1 = Optional.empty();
    private Optional<Integer> s2 = Optional.empty();
    
    private Optional<Long> h1 = Optional.empty();
    private Optional<Long> h2 = Optional.empty();
    private Optional<Long> h3 = Optional.empty();
    private Optional<Long> h4 = Optional.empty();

    public Builder addAddress(final InetNetwork address) {
      addresses.add(address);
      return this;
    }

    public Builder addAddresses(final Collection<InetNetwork> addresses) {
      this.addresses.addAll(addresses);
      return this;
    }

    public Builder addDnsServer(final InetAddress dnsServer) {
      dnsServers.add(dnsServer);
      return this;
    }

    public Builder addDnsServers(final Collection<? extends InetAddress> dnsServers) {
      this.dnsServers.addAll(dnsServers);
      return this;
    }

    public Interface build() throws BadConfigException {
      if (keyPair == null)
        throw new BadConfigException(
            Section.INTERFACE, Location.PRIVATE_KEY, Reason.MISSING_ATTRIBUTE, null);
      if (!includedApplications.isEmpty() && !excludedApplications.isEmpty())
        throw new BadConfigException(
            Section.INTERFACE, Location.INCLUDED_APPLICATIONS, Reason.INVALID_KEY, null);
      return new Interface(this);
    }

    public Builder excludeApplication(final String application) {
      excludedApplications.add(application);
      return this;
    }

    public Builder excludeApplications(final Collection<String> applications) {
      excludedApplications.addAll(applications);
      return this;
    }

    public Builder includeApplication(final String application) {
      includedApplications.add(application);
      return this;
    }

    public Builder includeApplications(final Collection<String> applications) {
      includedApplications.addAll(applications);
      return this;
    }

    public Builder parseAddresses(final CharSequence addresses) throws BadConfigException {
      try {
        for (final String address : Attribute.split(addresses))
          addAddress(InetNetwork.parse(address));
        return this;
      } catch (final ParseException e) {
        throw new BadConfigException(Section.INTERFACE, Location.ADDRESS, e);
      }
    }

    public Builder parseDnsServers(final CharSequence dnsServers) throws BadConfigException {
      try {
        for (final String dnsServer : Attribute.split(dnsServers))
          addDnsServer(InetAddresses.parse(dnsServer));
        return this;
      } catch (final ParseException e) {
        throw new BadConfigException(Section.INTERFACE, Location.DNS, e);
      }
    }

    public Builder parseExcludedApplications(final CharSequence apps) {
      return excludeApplications(List.of(Attribute.split(apps)));
    }

    public Builder parseIncludedApplications(final CharSequence apps) {
      return includeApplications(List.of(Attribute.split(apps)));
    }

    public Builder parseListenPort(final String listenPort) throws BadConfigException {
      try {
        return setListenPort(Integer.parseInt(listenPort));
      } catch (final NumberFormatException e) {
        throw new BadConfigException(Section.INTERFACE, Location.LISTEN_PORT, listenPort, e);
      }
    }

    public Builder parseMtu(final String mtu) throws BadConfigException {
      try {
        return setMtu(Integer.parseInt(mtu));
      } catch (final NumberFormatException e) {
        throw new BadConfigException(Section.INTERFACE, Location.MTU, mtu, e);
      }
    }

    public Builder parseJc(final String jc) throws BadConfigException {
      try {
        return setJc(Integer.parseInt(jc));
      } catch (final NumberFormatException e) {
        throw new BadConfigException(Section.INTERFACE, Location.JC, jc, e);
      }
    }
    
    public Builder parseJmax(final String jmax) throws BadConfigException {
      try {
        return setJmax(Integer.parseInt(jmax));
      } catch (final NumberFormatException e) {
        throw new BadConfigException(Section.INTERFACE, Location.JMAX, jmax, e);
      }
    }

    public Builder parseJmin(final String jmin) throws BadConfigException {
      try {
        return setJmin(Integer.parseInt(jmin));
      } catch (final NumberFormatException e) {
        throw new BadConfigException(Section.INTERFACE, Location.JMIN, jmin, e);
      }
    }    

    public Builder parseS1(final String s1) throws BadConfigException {
      try {
        return setS1(Integer.parseInt(s1));
      } catch (final NumberFormatException e) {
        throw new BadConfigException(Section.INTERFACE, Location.S1, s1, e);
      }
    } 

    public Builder parseS2(final String s2) throws BadConfigException {
      try {
        return setS2(Integer.parseInt(s2));
      } catch (final NumberFormatException e) {
        throw new BadConfigException(Section.INTERFACE, Location.S2, s2, e);
      }
    } 

    public Builder parseH1(final String h1) throws BadConfigException {
      try {
        return setH1(Long.parseLong(h1));
      } catch (final NumberFormatException e) {
        throw new BadConfigException(Section.INTERFACE, Location.H1, h1, e);
      }
    } 

    public Builder parseH2(final String h2) throws BadConfigException {
      try {
        return setH2(Long.parseLong(h2));
      } catch (final NumberFormatException e) {
        throw new BadConfigException(Section.INTERFACE, Location.H2, h2, e);
      }
    } 

    public Builder parseH3(final String h3) throws BadConfigException {
      try {
        return setH3(Long.parseLong(h3));
      } catch (final NumberFormatException e) {
        throw new BadConfigException(Section.INTERFACE, Location.H3, h3, e);
      }
    } 

    public Builder parseH4(final String h4) throws BadConfigException {
      try {
        return setH4(Long.parseLong(h4));
      } catch (final NumberFormatException e) {
        throw new BadConfigException(Section.INTERFACE, Location.H4, h4, e);
      }
    } 

    public Builder parsePrivateKey(final String privateKey) throws BadConfigException {
      try {
        return setKeyPair(new KeyPair(Key.fromBase64(privateKey)));
      } catch (final KeyFormatException e) {
        throw new BadConfigException(Section.INTERFACE, Location.PRIVATE_KEY, e);
      }
    }

    public Builder setKeyPair(final KeyPair keyPair) {
      this.keyPair = keyPair;
      return this;
    }

    public Builder setListenPort(final int listenPort) throws BadConfigException {
      if (listenPort < MIN_UDP_PORT || listenPort > MAX_UDP_PORT)
        throw new BadConfigException(Section.INTERFACE, Location.LISTEN_PORT, Reason.INVALID_VALUE,
            String.valueOf(listenPort));
      this.listenPort = listenPort == 0 ? Optional.empty() : Optional.of(listenPort);
      return this;
    }

    public Builder setMtu(final int mtu) throws BadConfigException {
      if (mtu < 0)
        throw new BadConfigException(
            Section.INTERFACE, Location.MTU, Reason.INVALID_VALUE, String.valueOf(mtu));
      this.mtu = mtu == 0 ? Optional.empty() : Optional.of(mtu);
      return this;
    }
    
    public Builder setJc(final int jc) throws BadConfigException {
      if (jc < 0)
        throw new BadConfigException(
            Section.INTERFACE, Location.JC, Reason.INVALID_VALUE, String.valueOf(jc));
      this.jc = jc == 0 ? Optional.empty() : Optional.of(jc);
      return this;
    }      
    
    public Builder setJmin(final int jmin) throws BadConfigException {
      if (jmin < 0)
        throw new BadConfigException(
            Section.INTERFACE, Location.JMIN, Reason.INVALID_VALUE, String.valueOf(jmin));
      this.jmin = jmin == 0 ? Optional.empty() : Optional.of(jmin);
      return this;
    }

    public Builder setJmax(final int jmax) throws BadConfigException {
      if (jmax < 0)
        throw new BadConfigException(
            Section.INTERFACE, Location.JMAX, Reason.INVALID_VALUE, String.valueOf(jmax));
      this.jmax = jmax == 0 ? Optional.empty() : Optional.of(jmax);
      return this;
    }

    public Builder setS1(final int s1) throws BadConfigException {
      if (s1 < 0)
        throw new BadConfigException(
            Section.INTERFACE, Location.S1, Reason.INVALID_VALUE, String.valueOf(s1));
      this.s1 = s1 == 0 ? Optional.empty() : Optional.of(s1);
      return this;
    }

    public Builder setS2(final int s2) throws BadConfigException {
      if (s2 < 0)
        throw new BadConfigException(
            Section.INTERFACE, Location.S2, Reason.INVALID_VALUE, String.valueOf(s2));
      this.s2 = s2 == 0 ? Optional.empty() : Optional.of(s2);
      return this;
    }

    public Builder setH1(final long h1) throws BadConfigException {
      if (h1 < 0)
        throw new BadConfigException(
            Section.INTERFACE, Location.H1, Reason.INVALID_VALUE, String.valueOf(h1));
      this.h1 = h1 == 0 ? Optional.empty() : Optional.of(h1);
      return this;
    }

    public Builder setH2(final long h2) throws BadConfigException {
      if (h2 < 0)
        throw new BadConfigException(
            Section.INTERFACE, Location.H2, Reason.INVALID_VALUE, String.valueOf(h2));
      this.h2 = h2 == 0 ? Optional.empty() : Optional.of(h2);
      return this;
    }

    public Builder setH3(final long h3) throws BadConfigException {
      if (h3 < 0)
        throw new BadConfigException(
            Section.INTERFACE, Location.H3, Reason.INVALID_VALUE, String.valueOf(h3));
      this.h3 = h3 == 0 ? Optional.empty() : Optional.of(h3);
      return this;
    }

    public Builder setH4(final long h4) throws BadConfigException {
      if (h4 < 0)
        throw new BadConfigException(
            Section.INTERFACE, Location.H4, Reason.INVALID_VALUE, String.valueOf(h4));
      this.h4 = h4 == 0 ? Optional.empty() : Optional.of(h4);
      return this;
    }
  }
}    
