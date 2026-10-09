# System DNS preservation

This patch adds an opt-in **Use system DNS** setting for desktop Windows and
macOS with AmneziaWG/WireGuard. It is disabled by default. Other protocols
reject connection preparation while the setting is enabled; mobile platforms
and macOS Network Extension do not expose this mode.

The daemon reads DNS addresses at each activation after tearing down the old
connection, skips resolver replacement, and allows those addresses through the
existing DNS firewall rules. DNS route exclusions are kept separate from
general traffic exclusions, so they do not grant access to other ports on DNS
servers. Switching modes and reconnecting remove the previous DNS routes.
Configured application/imported DNS values are retained for use when this mode
is disabled. The UI explicitly warns that DNS may leave the VPN.

## Automated checks

The tests compile the production daemon, IPC parser, WireGuard configuration
serializer, and native DNS reader. Network and firewall operations use fake
backends. These tests never connect a VPN or modify system networking.

With Qt 6.10+ and a C++17 toolchain:

```sh
cmake -S tests/systemdns -B build-systemdns -DCMAKE_PREFIX_PATH=/path/to/Qt
cmake --build build-systemdns --config Release
ctest --test-dir build-systemdns -C Release --output-on-failure
```

Ensure the Qt runtime libraries are on PATH on Windows. Set
`AMNEZIA_TEST_SYSTEM_DNS=1` to also run the native resolver discovery check; it
only reads the current system DNS configuration.

## Integration validation matrix

The Windows client and service were built in Release mode with Qt 6.10.3 and
MSVC 19.44. The automated suite reported 19 passes (including initialization
and cleanup), with native Windows DNS discovery enabled. A local tester ran
the matching patched client and service and reported the VPN/DNS scenario
working. This does not independently certify every scenario below. Native
macOS build and runtime validation remain outstanding.

Use a disposable host or VM with a test VPN endpoint and record DNS
configuration, routes, and firewall state before and after each scenario:

Install the matching patched client **and** service. An older service does not
understand the new mode flag; running only the new UI is not a valid test.

- Connect with system DNS enabled; resolve a hostname through the OS and query
  every configured DNS directly over UDP and TCP port 53.
- Check IPv4, IPv6, loopback/local resolvers, DHCP and manually assigned DNS.
- Confirm DNS resolver/search-domain configuration is unchanged.
- Disconnect, reconnect, switch servers, sleep/wake and change networks;
  verify old DNS routes and firewall exceptions disappear.
- Disable the setting and confirm the existing application DNS mode works.
- With Kill Switch enabled, confirm non-DNS traffic to a DNS server is still
  blocked outside the tunnel and the usual disconnected protection remains.
- Verify both full tunnel and supported site/application split-tunnel modes.
- Repeat on native macOS. A Windows build does not validate its platform code.

## Limits of this first implementation

The native readers enumerate adapter/network-service DNS servers. Corporate
NRPT rules, macOS `/etc/resolver` overrides, arbitrary DNS ports, and local
proxies requiring outbound DoH/DoT traffic need separate integration work.
macOS uses the existing default-route exclusion mechanism for remote DNS;
non-default uplinks and complex split-DNS routing have not been validated.
System configuration is preserved, but reachability under Kill Switch must
not be inferred for these cases from the unit tests.
