// tapbridge connects a dedicated TAP-Windows adapter to the active XRay SOCKS
// inbound. Configuration arrives on stdin, never on the process command line.
package main

import (
	"bytes"
	"context"
	"crypto/tls"
	"encoding/binary"
	"encoding/json"
	"fmt"
	"io"
	"net"
	"net/http"
	"net/netip"
	"os"
	"strings"
	"sync"
	"sync/atomic"
	"time"

	"github.com/songgao/water"
	"github.com/xjasonlyu/tun2socks/v2/core"
	"github.com/xjasonlyu/tun2socks/v2/core/adapter"
	"github.com/xjasonlyu/tun2socks/v2/core/device/iobased"
	M "github.com/xjasonlyu/tun2socks/v2/metadata"
	"github.com/xjasonlyu/tun2socks/v2/proxy"
	"github.com/xjasonlyu/tun2socks/v2/tunnel"
)

// shareHandler sends DNS-over-UDP requests through Cloudflare DoH over TCP.
// Other UDP protocols, including QUIC on port 443, use the XRay SOCKS
// UDP_ASSOCIATE path through tun2socks.
type shareHandler struct {
	doh         *http.Client
	dohFallback *http.Client
}

func newShareHandler(socks *proxy.Socks5) *shareHandler {
	newClient := func(serverName, address string) *http.Client {
		transport := &http.Transport{
			TLSClientConfig: &tls.Config{ServerName: serverName, MinVersion: tls.VersionTLS12},
			DialContext: func(ctx context.Context, _, _ string) (net.Conn, error) {
				return socks.DialContext(ctx, &M.Metadata{
					Network: M.TCP,
					DstIP:   netip.MustParseAddr(address),
					DstPort: 443,
				})
			},
		}
		return &http.Client{Transport: transport, Timeout: 12 * time.Second}
	}
	return &shareHandler{
		doh:         newClient("cloudflare-dns.com", "1.1.1.1"),
		dohFallback: newClient("dns.google", "8.8.8.8"),
	}
}

func (h *shareHandler) HandleTCP(conn adapter.TCPConn) {
	tunnel.T().HandleTCP(conn)
}

func (h *shareHandler) HandleUDP(conn adapter.UDPConn) {
	destinationPort := conn.ID().LocalPort
	if destinationPort == 53 {
		go h.handleDNS(conn)
		return
	}
	tunnel.T().HandleUDP(conn)
}

func (h *shareHandler) handleDNS(conn adapter.UDPConn) {
	defer conn.Close()
	buf := make([]byte, 4096)
	for {
		n, peer, err := conn.ReadFrom(buf)
		if err != nil {
			return
		}
		if response, ok := ipv4OnlyAAAAResponse(buf[:n]); ok {
			_, _ = conn.WriteTo(response, peer)
			continue
		}
		body, err := h.forwardDNS(buf[:n])
		if err == nil {
			_, _ = conn.WriteTo(body, peer)
		}
	}
}

// ipv4OnlyAAAAResponse returns an NODATA response for an AAAA question. The
// sharing bridge carries IPv4 only; handing clients IPv6 addresses would make
// some connections stall on an unusable path instead of selecting IPv4.
func ipv4OnlyAAAAResponse(query []byte) ([]byte, bool) {
	if len(query) < 17 || binary.BigEndian.Uint16(query[4:6]) != 1 {
		return nil, false
	}
	offset := 12
	for {
		if offset >= len(query) {
			return nil, false
		}
		label := query[offset]
		switch label & 0xc0 {
		case 0x00:
			offset++
			if label == 0 {
				goto questionType
			}
			if label > 63 || offset+int(label) > len(query) {
				return nil, false
			}
			offset += int(label)
		case 0xc0:
			if offset+1 >= len(query) {
				return nil, false
			}
			offset += 2
			goto questionType
		default:
			return nil, false
		}
	}

questionType:
	if offset+4 > len(query) || binary.BigEndian.Uint16(query[offset:offset+2]) != 28 {
		return nil, false
	}
	response := make([]byte, offset+4)
	copy(response[0:2], query[0:2])
	// Preserve opcode, RD, and CD; mark response and recursion available.
	response[2] = 0x80 | (query[2] & 0x79)
	response[3] = 0x80 | (query[3] & 0x10)
	binary.BigEndian.PutUint16(response[4:6], 1)
	copy(response[12:], query[12:offset+4])
	return response, true
}

func (h *shareHandler) forwardDNS(query []byte) ([]byte, error) {
	resolvers := []struct {
		client *http.Client
		url    string
		host   string
	}{
		{h.doh, "https://1.1.1.1/dns-query", "cloudflare-dns.com"},
		{h.dohFallback, "https://8.8.8.8/dns-query", "dns.google"},
	}
	var lastErr error
	for _, resolver := range resolvers {
		if resolver.client == nil {
			continue
		}
		answer, err := forwardDNSDoH(query, resolver.client, resolver.url, resolver.host)
		if err == nil {
			return answer, nil
		}
		lastErr = err
	}
	if lastErr == nil {
		lastErr = fmt.Errorf("no DNS-over-HTTPS resolver is configured")
	}
	return nil, fmt.Errorf("all DNS-over-HTTPS resolvers failed: %w", lastErr)
}

func forwardDNSDoH(query []byte, client *http.Client, url, host string) ([]byte, error) {
	request, err := http.NewRequest(http.MethodPost, url, bytes.NewReader(query))
	if err != nil {
		return nil, err
	}
	request.Host = host
	request.Header.Set("Content-Type", "application/dns-message")
	request.Header.Set("Accept", "application/dns-message")
	response, err := client.Do(request)
	if err != nil {
		return nil, err
	}
	defer response.Body.Close()
	body, err := io.ReadAll(io.LimitReader(response.Body, 4097))
	if err != nil {
		return nil, err
	}
	if response.StatusCode != http.StatusOK {
		return nil, fmt.Errorf("DoH returned HTTP %s", response.Status)
	}
	if len(body) > 4096 {
		return nil, fmt.Errorf("DoH response exceeds the UDP DNS size limit")
	}
	return body, nil
}

type config struct {
	Adapter   string `json:"adapter"`
	Address   string `json:"address"`
	Port      int    `json:"port"`
	Username  string `json:"username"`
	Password  string `json:"password"`
	ReadyFile string `json:"readyFile"`
	TraceFile string `json:"traceFile"`
}

var currentStage atomic.Value
var tracePath atomic.Value
var readySignal chan struct{}
var readyOnce sync.Once

const bridgeMTU = 1280

func trace(stage string) {
	currentStage.Store(stage)
	if value := tracePath.Load(); value != nil {
		line := time.Now().Format(time.RFC3339Nano) + " " + stage + "\r\n"
		file, err := os.OpenFile(value.(string), os.O_CREATE|os.O_APPEND|os.O_WRONLY, 0600)
		if err == nil {
			_, _ = file.WriteString(line)
			_ = file.Close()
		}
	}
}

type counted struct {
	io.ReadWriter
	read, written atomic.Uint64
}

func (c *counted) Read(b []byte) (int, error) {
	n, e := c.ReadWriter.Read(b)
	if n > 0 {
		c.read.Add(1)
	}
	return n, e
}
func (c *counted) Write(b []byte) (int, error) {
	n, e := c.ReadWriter.Write(b)
	if n > 0 {
		c.written.Add(1)
	}
	return n, e
}

func run() error {
	trace("configuration-read")
	var cfg config
	decoder := json.NewDecoder(os.Stdin)
	if err := decoder.Decode(&cfg); err != nil {
		return fmt.Errorf("configuration: %w", err)
	}
	if cfg.TraceFile != "" {
		tracePath.Store(cfg.TraceFile)
	}
	trace("configuration-validated")
	if cfg.Adapter != "Amnezia Share" || cfg.Address != "127.0.0.1" || cfg.Port < 1 || cfg.Port > 65535 || cfg.ReadyFile == "" {
		return fmt.Errorf("invalid dedicated adapter or local XRay endpoint")
	}
	trace("creating-socks-dialer")
	socks, err := proxy.NewSocks5(net.JoinHostPort(cfg.Address, fmt.Sprint(cfg.Port)), cfg.Username, cfg.Password)
	if err != nil {
		return err
	}
	trace("opening-tap-device")
	tap, err := water.New(water.Config{DeviceType: water.TUN, PlatformSpecificParams: water.PlatformSpecificParams{
		ComponentID: "tap0901", InterfaceName: cfg.Adapter, Network: "10.254.254.1/24",
	}})
	if err != nil {
		return err
	}
	trace("tap-device-open")
	defer tap.Close()
	packets := &counted{ReadWriter: tap}
	defer func() {
		fmt.Fprintf(os.Stderr, "packets received=%d returned=%d\n", packets.read.Load(), packets.written.Load())
	}()
	// Share.ps1 configures the TAP interface to this conservative MTU to keep
	// packets within the XRay/SOCKS path. The packet stack must use the same MTU;
	// otherwise it can emit 1500-byte frames that Windows drops on the 1280-byte
	// TAP interface, which looks like pages starting and then stalling.
	link, err := iobased.New(packets, bridgeMTU, 0)
	if err != nil {
		return err
	}
	trace("creating-packet-stack")
	tunnel.T().SetDialer(socks)
	stack, err := core.CreateStack(&core.Config{LinkEndpoint: link, TransportHandler: newShareHandler(socks)})
	if err != nil {
		return err
	}
	trace("packet-stack-ready")
	defer stack.Close()
	trace("writing-ready-marker")
	if err := os.WriteFile(cfg.ReadyFile, []byte("READY"), 0600); err != nil {
		return fmt.Errorf("write readiness marker: %w", err)
	}
	trace("ready")
	readyOnce.Do(func() { close(readySignal) })
	// EOF on the inherited pipe means the owning service has exited.
	_, err = io.Copy(io.Discard, io.MultiReader(decoder.Buffered(), os.Stdin))
	return err
}

func main() {
	readySignal = make(chan struct{})
	done := make(chan error, 1)
	go func() { done <- run() }()
	select {
	case <-readySignal:
		// The startup watchdog ends once the bridge is operational. The bridge
		// remains alive until its owner closes stdin during normal shutdown.
		if err := <-done; err != nil {
			trace("error: " + strings.ReplaceAll(err.Error(), "\n", " "))
			fmt.Fprintln(os.Stderr, err)
			os.Exit(1)
		}
	case err := <-done:
		if err != nil {
			trace("error: " + strings.ReplaceAll(err.Error(), "\n", " "))
			fmt.Fprintln(os.Stderr, err)
			os.Exit(1)
		}
	case <-time.After(35 * time.Second):
		stage := "unknown"
		if value := currentStage.Load(); value != nil {
			stage = value.(string)
		}
		trace("startup-timeout-at-" + stage)
		fmt.Fprintf(os.Stderr, "TAP bridge startup timed out at %s\n", stage)
		os.Exit(2)
	}
}
