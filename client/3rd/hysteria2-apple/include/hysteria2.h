// hysteria2.h — Public C API for the Hysteria2 client library (iOS).
// This header is embedded in Hysteria2.xcframework and imported through
// WireGuardNetworkExtension-Bridging-Header.h.
//
// The library is built from client/3rd/hysteria2-apple using build-ios.sh.

#ifndef HYSTERIA2_H
#define HYSTERIA2_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Socket callback type.
// Called by the Hysteria2 client for every UDP socket it creates.
// The host (iOS Network Extension) uses this to bind the socket to the
// physical network interface via setsockopt(IP_BOUND_IF / IPV6_BOUND_IF),
// preventing traffic from leaking into the VPN tunnel.
typedef void (*libhysteria2_sockcallback)(uintptr_t fd, void* ctx);

// LibHysteria2SetSockCallback registers the socket callback.
// Pass NULL to deregister.
// Thread-safe; may be called before LibHysteria2RunClient.
void LibHysteria2SetSockCallback(libhysteria2_sockcallback cb, void* ctx);

// LibHysteria2RunClient starts the Hysteria2 client.
// configPath must point to a UTF-8 encoded YAML config file
// (see PacketTunnelProvider+Hysteria2.swift for the expected format).
// This function blocks until LibHysteria2StopClient() is called.
void LibHysteria2RunClient(const char* configPath);

// LibHysteria2StopClient stops the running client.
// Safe to call even if no client is running.
void LibHysteria2StopClient(void);

#ifdef __cplusplus
}
#endif

#endif // HYSTERIA2_H
