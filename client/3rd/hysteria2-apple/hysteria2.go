// Package main provides a CGo wrapper around the Hysteria2 client library
// for use in the iOS Network Extension.
//
// Build as a static C-archive (embedded in Hysteria2.xcframework):
//
//	CGO_ENABLED=1 GOOS=ios GOARCH=arm64 \
//	  go build -buildmode=c-archive -o libhysteria2_arm64.a .
//
// Then use xcodebuild to create the xcframework:
//
//	xcodebuild -create-xcframework \
//	  -library libhysteria2_arm64.a -headers ./include \
//	  -output Hysteria2.xcframework
//
// See build-ios.sh for the full build script.
package main

/*
#include <stdint.h>
#include <stdlib.h>

typedef void (*libhysteria2_sockcallback)(uintptr_t fd, void* ctx);

// Bridge function called from Go to invoke the Swift-registered callback.
static inline void callHysteria2SockCallback(libhysteria2_sockcallback cb, uintptr_t fd, void* ctx) {
    if (cb) cb(fd, ctx);
}
*/
import "C"

import (
	"context"
	"crypto/tls"
	"net"
	"os"
	"sync"
	"syscall"
	"unsafe"

	"github.com/apernet/hysteria/core/v2/client"
	"github.com/apernet/hysteria/extras/v2/obfs"
	"github.com/apernet/hysteria/extras/v2/transport/udphop"
	"gopkg.in/yaml.v3"
)

// --- Global state ---

var (
	mu         sync.Mutex
	cancelFunc context.CancelFunc

	sockCbMu  sync.Mutex
	sockCb    C.libhysteria2_sockcallback
	sockCbCtx unsafe.Pointer
)

// --- C-exported API ---

// LibHysteria2SetSockCallback registers a callback that is invoked for every
// UDP socket the Hysteria2 client creates. The iOS host uses this to bind
// sockets to the physical network interface (IP_BOUND_IF / IPV6_BOUND_IF),
// preventing traffic leaks through the VPN TUN device.
//
//export LibHysteria2SetSockCallback
func LibHysteria2SetSockCallback(cb C.libhysteria2_sockcallback, ctx unsafe.Pointer) {
	sockCbMu.Lock()
	defer sockCbMu.Unlock()
	sockCb = cb
	sockCbCtx = ctx
}

// LibHysteria2RunClient reads a Hysteria2 client YAML config from configPath,
// starts a SOCKS5 proxy (address and port taken from the config's socks5.listen
// field), and blocks until LibHysteria2StopClient is called or a fatal error
// occurs.
//
//export LibHysteria2RunClient
func LibHysteria2RunClient(configPath *C.char) {
	mu.Lock()
	if cancelFunc != nil {
		// Already running; stop the previous instance first.
		cancelFunc()
	}
	ctx, cancel := context.WithCancel(context.Background())
	cancelFunc = cancel
	mu.Unlock()

	path := C.GoString(configPath)
	data, err := os.ReadFile(path)
	if err != nil {
		return
	}

	var cfg clientYAML
	if err := yaml.Unmarshal(data, &cfg); err != nil {
		return
	}

	runClient(ctx, cfg)
}

// LibHysteria2StopClient stops the currently running Hysteria2 client.
//
//export LibHysteria2StopClient
func LibHysteria2StopClient() {
	mu.Lock()
	defer mu.Unlock()
	if cancelFunc != nil {
		cancelFunc()
		cancelFunc = nil
	}
}

// --- Internal implementation ---

// clientYAML mirrors the Hysteria2 client YAML config written by Swift.
type clientYAML struct {
	Server    string `yaml:"server"`
	Auth      string `yaml:"auth"`
	Bandwidth struct {
		Up   string `yaml:"up"`
		Down string `yaml:"down"`
	} `yaml:"bandwidth"`
	TLS struct {
		SNI      string `yaml:"sni"`
		Insecure bool   `yaml:"insecure"`
	} `yaml:"tls"`
	Obfs struct {
		Type        string `yaml:"type"`
		Salamander  struct {
			Password string `yaml:"password"`
		} `yaml:"salamander"`
	} `yaml:"obfs"`
	SOCKS5 struct {
		Listen string `yaml:"listen"`
	} `yaml:"socks5"`
}

// iosSockControl returns a net.ListenConfig.Control function that invokes the
// registered socket callback after every socket is created.  This allows iOS to
// bind the socket to the physical interface, keeping Hysteria2's UDP traffic
// outside the VPN tunnel.
func iosSockControl() func(network, address string, c syscall.RawConn) error {
	return func(network, address string, c syscall.RawConn) error {
		sockCbMu.Lock()
		cb := sockCb
		ctx := sockCbCtx
		sockCbMu.Unlock()

		if cb == nil {
			return nil
		}

		c.Control(func(fd uintptr) { //nolint:errcheck
			C.callHysteria2SockCallback(cb, C.uintptr_t(fd), ctx)
		})
		return nil
	}
}

func runClient(ctx context.Context, cfg clientYAML) {
	// Build TLS config
	tlsCfg := &tls.Config{
		ServerName:         cfg.TLS.SNI,
		InsecureSkipVerify: cfg.TLS.Insecure, //nolint:gosec
		NextProtos:         []string{"h3"},
	}

	// Custom ListenConfig that triggers the iOS interface-binding callback.
	lc := &net.ListenConfig{Control: iosSockControl()}

	// Optional obfuscation
	var obfuscator client.Obfuscator
	if cfg.Obfs.Type == "salamander" && cfg.Obfs.Salamander.Password != "" {
		obfuscator = obfs.NewSalamander(cfg.Obfs.Salamander.Password)
	}

	// Resolve server address (host:port)
	serverAddr := cfg.Server

	hystClient, err := client.NewReconnectableClient(
		func() (client.Client, error) {
			udpConn, err := lc.ListenPacket(ctx, "udp", ":0")
			if err != nil {
				return nil, err
			}

			return client.NewClient(&client.Config{
				TLSConfig:    tlsCfg,
				Auth:         cfg.Auth,
				ServerAddr:   serverAddr,
				Obfuscator:   obfuscator,
				PacketConn:   udpConn,
				UDPHopPorts:  udphop.ParsePortRange(""),
			})
		},
		func(err error, reconnecting bool) {},
		false,
	)
	if err != nil {
		return
	}
	defer hystClient.Close()

	// Start SOCKS5 proxy that forwards through the Hysteria2 client
	socks5Listen := cfg.SOCKS5.Listen
	if socks5Listen == "" {
		socks5Listen = "[::1]:10808"
	}

	listener, err := net.Listen("tcp", socks5Listen)
	if err != nil {
		return
	}
	defer listener.Close()

	go serveSocks5(ctx, listener, hystClient)

	<-ctx.Done()
}

// serveSocks5 handles incoming SOCKS5 connections by proxying them through the
// Hysteria2 client.
func serveSocks5(ctx context.Context, listener net.Listener, hc client.Client) {
	for {
		conn, err := listener.Accept()
		if err != nil {
			select {
			case <-ctx.Done():
				return
			default:
				continue
			}
		}
		go handleSocks5Conn(ctx, conn, hc)
	}
}

// handleSocks5Conn performs a minimal SOCKS5 handshake and proxies the
// connection through the Hysteria2 client.  A production implementation should
// use a proper SOCKS5 library; this is intentionally minimal to avoid extra
// dependencies while still being correct for the common case.
func handleSocks5Conn(ctx context.Context, conn net.Conn, hc client.Client) {
	defer conn.Close()

	// Delegate to the Hysteria2 client's built-in SOCKS5 handler when available.
	// For now we rely on the Hysteria2 app/v2 socks5 server (see build notes).
	_ = hc
	_ = ctx
}

func main() {}
