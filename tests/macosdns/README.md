# macOS DNS recovery regression tests

Run on macOS with CMake, Xcode command-line tools, and Qt 6 (Core, Network, Test):

```sh
cmake -S tests/macosdns -B build/macosdns -DCMAKE_PREFIX_PATH=/path/to/Qt
cmake --build build/macosdns --parallel
ctest --test-dir build/macosdns --output-on-failure
```

This standalone target does not need the application's Conan dependencies. It
compiles the production `DnsUtilsMacos` implementation and redirects only its
SystemConfiguration calls to an in-memory store. Qt event delivery, serial
libdispatch queues, and CoreFoundation values/reference counting are real.
Logging uses a lightweight adapter to avoid application IPC dependencies. Tests
never read or modify host DNS and require no root access.

Coverage includes deleted/replaced DNS overrides, new/deleted network services,
original static/DHCP DNS restoration, idempotent notifications, write/setup errors,
queued work across disconnect/reconnect, and late callbacks after destruction.
The `macos_dns_native_compile` object target also compiles the changed production
class with the real SystemConfiguration declarations and production logger.

This suite does not install or run a replacement VPN service. It does not prove
configd notification delivery or end-to-end recovery after USB attach, DHCP renew,
sleep/wake, or Wi-Fi changes. Before shipping, validate those events with a locally
built service and confirm both `scutil --dns` recovery while connected and original
DNS restoration after disconnect. No test should alter the user's active network.
