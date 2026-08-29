package dnsttclient

// Amnezia integration layer: builds a complete dnstt client (DNS transport →
// KCP → Noise NK → smux) and exposes it as a start/stop pair driven from the
// Android service. The DNS/KCP/Noise/smux composition mirrors dnstt-client's
// run(), the difference being that connections are fed from a gVisor netstack
// attached to the VpnService TUN rather than from a local TCP listener.

import (
	"context"
	"crypto/tls"
	"errors"
	"fmt"
	"io"
	"log"
	"net"
	"net/http"
	"net/url"
	"strings"
	"sync"
	"sync/atomic"
	"syscall"
	"time"

	utls "github.com/refraction-networking/utls"
	"github.com/xtaci/kcp-go/v5"
	"github.com/xtaci/smux"

	"org.amnezia.vpn/dnstt/dns"
	"org.amnezia.vpn/dnstt/noise"
	"org.amnezia.vpn/dnstt/turbotunnel"
)

const (
	// smux streams are closed after this much time without receiving data.
	idleTimeout = 2 * time.Minute

	// minMtu is the smallest tunnel payload dnstt is willing to operate on.
	minMtu = 80

	// handshakeTimeout bounds the Noise handshake so that a dead resolver
	// fails over to the next one instead of hanging forever.
	handshakeTimeout = 30 * time.Second

	// dialTimeoutTCP bounds a single TCP/TLS dial to a resolver.
	dialTimeoutTCP = 20 * time.Second

	// defaultUTLSDistribution is dnstt-client's default -utls value.
	defaultUTLSDistribution = "4*random,3*Firefox_120,1*Firefox_105,3*Chrome_120,1*Chrome_102,1*iOS_14,1*iOS_13"

	// Reconnect backoff bounds. A DNS tunnel loses its session whenever the
	// resolver path breaks, which is common enough that it must recover on
	// its own instead of leaving a dead tunnel behind a "connected" UI.
	initialBackoff = 2 * time.Second
	maxBackoff     = 60 * time.Second

	// downLogInterval throttles "tunnel is down" logging: applications retry
	// DNS several times per second, and one line per attempt buries the log.
	downLogInterval = 15 * time.Second
)

// Tunnel states reported to the Android layer.
const (
	StateConnected    = "connected"
	StateReconnecting = "reconnecting"
	StateDisconnected = "disconnected"
)

// errTunnelDown is returned by openStream while there is no live session, so
// callers can fail fast and quietly instead of logging every attempt.
var errTunnelDown = errors.New("dnstt: tunnel is down")

// ProtectFunc excludes a socket from the VPN routes. On Android it is backed by
// VpnService.protect. Returning false aborts the dial.
type ProtectFunc func(fd int) bool

// StateFunc reports tunnel state transitions to the host application.
type StateFunc func(state string)

// Config describes one dnstt tunnel.
type Config struct {
	// TunFd is the VpnService TUN descriptor. The Client takes ownership and
	// closes it on Stop.
	TunFd int

	// TunMtu is the MTU the TUN interface was established with. It is
	// unrelated to the dnstt payload MTU.
	TunMtu int

	// Domain is the DNS zone delegated to the dnstt server.
	Domain string

	// Resolvers is a comma-separated list of resolver specs, each optionally
	// weighted as in "3*https://a/dns-query,1*dot://b:853". Supported forms
	// are https:// (DoH), dot:// or tls:// (DoT) and udp:// or a bare
	// host:port (plain UDP DNS).
	Resolvers string

	// BootstrapIP, when set, is dialed in place of the resolver hostname
	// while the TLS SNI keeps the original name.
	BootstrapIP string

	// PubKeyHex is the dnstt server public key, 64 hex digits.
	PubKeyHex string

	// UTLSDistribution overrides the uTLS fingerprint distribution.
	UTLSDistribution string
}

// Client is a running dnstt tunnel.
type Client struct {
	cfg     Config
	protect ProtectFunc
	onState StateFunc

	ctx    context.Context
	cancel context.CancelFunc

	// downMu guards the throttled "tunnel is down" logging.
	downMu      sync.Mutex
	lastDownLog time.Time

	mu      sync.Mutex
	running bool

	transport net.PacketConn
	pconn     net.PacketConn
	kcpConn   *kcp.UDPSession
	sess      *smux.Session
	netStack  *netStack

	// liveSession mirrors sess for lock-free reads. Reconnecting holds mu for
	// as long as a Noise handshake takes, and packet handlers must not block
	// on that: they check here and fail fast instead.
	liveSession atomic.Pointer[smux.Session]
}

// dnsNameCapacity returns the number of bytes remaining for encoded data after
// including domain in a DNS name. Verbatim from dnstt-client.
func dnsNameCapacity(domain dns.Name) int {
	// Names must be 255 octets or shorter in total length.
	// https://tools.ietf.org/html/rfc1035#section-2.3.4
	capacity := 255
	// Subtract the length of the null terminator.
	capacity -= 1
	for _, label := range domain {
		// Subtract the length of the label and the length octet.
		capacity -= len(label) + 1
	}
	// Each label may be up to 63 bytes long and requires 64 bytes to encode.
	capacity = capacity * 63 / 64
	// Base32 expands every 5 bytes to 8.
	capacity = capacity * 5 / 8
	return capacity
}

// mtuForDomain returns the usable dnstt payload MTU for a parsed domain.
func mtuForDomain(domain dns.Name) int {
	// clientid + padding length prefix + padding + data length prefix
	return dnsNameCapacity(domain) - 8 - 1 - numPadding - 1
}

// CalculateMtu returns the dnstt payload MTU for a tunnel domain, or 0 if the
// domain is unusable. It is the single source of truth for the value shown in
// the UI and enforced at connect time.
func CalculateMtu(domainStr string) int {
	domain, err := dns.ParseName(strings.TrimSpace(domainStr))
	if err != nil {
		return 0
	}
	mtu := mtuForDomain(domain)
	if mtu < 0 {
		return 0
	}
	return mtu
}

// NewClient validates cfg and returns a Client that has not been started yet.
// onState may be nil.
func NewClient(cfg Config, protect ProtectFunc, onState StateFunc) (*Client, error) {
	if strings.TrimSpace(cfg.Domain) == "" {
		return nil, errors.New("dnstt: tunnel domain is empty")
	}
	if _, err := dns.ParseName(strings.TrimSpace(cfg.Domain)); err != nil {
		return nil, fmt.Errorf("dnstt: invalid tunnel domain %q: %w", cfg.Domain, err)
	}
	if _, err := noise.DecodeKey(strings.TrimSpace(cfg.PubKeyHex)); err != nil {
		return nil, fmt.Errorf("dnstt: invalid server public key: %w", err)
	}
	if len(resolverSpecs(cfg.Resolvers)) == 0 {
		return nil, errors.New("dnstt: no resolvers specified")
	}
	if cfg.TunMtu <= 0 {
		cfg.TunMtu = 1500
	}
	if strings.TrimSpace(cfg.UTLSDistribution) == "" {
		cfg.UTLSDistribution = defaultUTLSDistribution
	}
	return &Client{cfg: cfg, protect: protect, onState: onState}, nil
}

// notifyState reports a state transition, if the host asked to be told.
func (c *Client) notifyState(state string) {
	if c.onState != nil {
		c.onState(state)
	}
}

// logTunnelDown logs at most one line per downLogInterval while the tunnel is
// unusable, so a dead session does not flood the log.
func (c *Client) logTunnelDown(context string, err error) {
	c.downMu.Lock()
	defer c.downMu.Unlock()
	if time.Since(c.lastDownLog) < downLogInterval {
		return
	}
	c.lastDownLog = time.Now()
	log.Printf("dnstt: %s while the tunnel is down (%v); further such messages are suppressed for %s",
		context, err, downLogInterval)
}

// resolverSpecs splits a resolver list into non-empty entries, dropping weights.
func resolverSpecs(s string) []string {
	weights, labels, err := parseWeightedList(s)
	if err != nil {
		// Fall back to a plain comma split so that a resolver containing
		// characters the weighted-list grammar rejects still works.
		var out []string
		for _, part := range strings.Split(s, ",") {
			if part = strings.TrimSpace(part); part != "" {
				out = append(out, part)
			}
		}
		return out
	}
	var out []string
	for i, label := range labels {
		if label = strings.TrimSpace(label); label != "" && weights[i] > 0 {
			out = append(out, label)
		}
	}
	return out
}

// orderedResolvers returns the resolvers in weighted-random order, so that a
// failing resolver falls back to the next one instead of aborting the tunnel.
func orderedResolvers(s string) []string {
	weights, labels, err := parseWeightedList(s)
	if err != nil {
		return resolverSpecs(s)
	}

	type entry struct {
		weight uint32
		label  string
	}
	var pool []entry
	for i, label := range labels {
		if label = strings.TrimSpace(label); label != "" && weights[i] > 0 {
			pool = append(pool, entry{weights[i], label})
		}
	}

	var out []string
	for len(pool) > 0 {
		w := make([]uint32, len(pool))
		for i, e := range pool {
			w[i] = e.weight
		}
		i := sampleWeighted(w)
		out = append(out, pool[i].label)
		pool = append(pool[:i], pool[i+1:]...)
	}
	return out
}

// protectControl is a net.Dialer/net.ListenConfig Control hook that hands the
// raw socket to VpnService.protect before it is connected.
func (c *Client) protectControl(network, address string, rc syscall.RawConn) error {
	if c.protect == nil {
		return nil
	}
	var ok bool
	if err := rc.Control(func(fd uintptr) { ok = c.protect(int(fd)) }); err != nil {
		return err
	}
	if !ok {
		return fmt.Errorf("dnstt: protect() refused the socket for %s", address)
	}
	return nil
}

// dialContext dials a resolver with a protected socket, substituting the
// bootstrap IP for a resolver hostname when one is configured.
func (c *Client) dialContext(ctx context.Context, network, addr string) (net.Conn, error) {
	target := addr
	if bootstrap := strings.TrimSpace(c.cfg.BootstrapIP); bootstrap != "" {
		if host, port, err := net.SplitHostPort(addr); err == nil && net.ParseIP(host) == nil {
			target = net.JoinHostPort(bootstrap, port)
		}
	}
	d := &net.Dialer{Timeout: dialTimeoutTCP, Control: c.protectControl}
	return d.DialContext(ctx, network, target)
}

// buildTransport creates the underlying DNS transport for one resolver spec.
func (c *Client) buildTransport(spec string, helloID *utls.ClientHelloID) (net.Addr, net.PacketConn, error) {
	spec = strings.TrimSpace(spec)
	lower := strings.ToLower(spec)

	switch {
	case strings.HasPrefix(lower, "https://"):
		if err := parseDoHURL(spec); err != nil {
			return nil, nil, err
		}
		var rt http.RoundTripper
		if helloID == nil {
			transport := http.DefaultTransport.(*http.Transport).Clone()
			// Match utlsRoundTripper and DoT mode, which do not take a
			// proxy from the environment.
			transport.Proxy = nil
			transport.DialContext = c.dialContext
			rt = transport
		} else {
			rt = NewUTLSRoundTripper(nil, helloID, c.dialContext)
		}
		pconn, err := NewHTTPPacketConn(rt, spec, 32)
		if err != nil {
			return nil, nil, err
		}
		return turbotunnel.DummyAddr{}, pconn, nil

	case strings.HasPrefix(lower, "dot://"), strings.HasPrefix(lower, "tls://"):
		addr := spec[strings.Index(spec, "://")+3:]
		addr = withDefaultPort(addr, "853")
		var dialTLSContext func(ctx context.Context, network, addr string) (net.Conn, error)
		if helloID == nil {
			dialTLSContext = func(ctx context.Context, network, addr string) (net.Conn, error) {
				raw, err := c.dialContext(ctx, network, addr)
				if err != nil {
					return nil, err
				}
				host, _, err := net.SplitHostPort(addr)
				if err != nil {
					raw.Close()
					return nil, err
				}
				conn := tls.Client(raw, &tls.Config{ServerName: host})
				if err := conn.HandshakeContext(ctx); err != nil {
					raw.Close()
					return nil, err
				}
				return conn, nil
			}
		} else {
			dialTLSContext = func(ctx context.Context, network, addr string) (net.Conn, error) {
				return utlsDialContext(ctx, network, addr, nil, helloID, c.dialContext)
			}
		}
		pconn, err := NewTLSPacketConn(addr, dialTLSContext)
		if err != nil {
			return nil, nil, err
		}
		return turbotunnel.DummyAddr{}, pconn, nil

	default:
		addr := spec
		if strings.HasPrefix(lower, "udp://") {
			addr = spec[len("udp://"):]
		}
		addr = withDefaultPort(addr, "53")
		host, port, err := net.SplitHostPort(addr)
		if err != nil {
			return nil, nil, fmt.Errorf("dnstt: bad UDP resolver %q: %w", spec, err)
		}
		if net.ParseIP(host) == nil {
			// Resolving the resolver's own name would have to travel
			// through the tunnel that is not up yet.
			if bootstrap := strings.TrimSpace(c.cfg.BootstrapIP); bootstrap != "" {
				host = bootstrap
			} else {
				return nil, nil, fmt.Errorf("dnstt: UDP resolver %q must be an IP address, or a bootstrap IP must be set", spec)
			}
		}
		remoteAddr, err := net.ResolveUDPAddr("udp", net.JoinHostPort(host, port))
		if err != nil {
			return nil, nil, err
		}
		lc := net.ListenConfig{Control: c.protectControl}
		pconn, err := lc.ListenPacket(context.Background(), "udp", ":0")
		if err != nil {
			return nil, nil, err
		}
		return remoteAddr, pconn, nil
	}
}

// withDefaultPort appends defaultPort to hostPort if it carries no port.
func withDefaultPort(hostPort, defaultPort string) string {
	if _, _, err := net.SplitHostPort(hostPort); err == nil {
		return hostPort
	}
	return net.JoinHostPort(hostPort, defaultPort)
}

// Start brings the tunnel up and attaches the network stack to the TUN.
func (c *Client) Start() error {
	c.mu.Lock()
	defer c.mu.Unlock()

	if c.running {
		return errors.New("dnstt: client already running")
	}

	domain, err := dns.ParseName(strings.TrimSpace(c.cfg.Domain))
	if err != nil {
		return err
	}
	pubkey, err := noise.DecodeKey(strings.TrimSpace(c.cfg.PubKeyHex))
	if err != nil {
		return err
	}

	mtu := mtuForDomain(domain)
	if mtu < minMtu {
		return fmt.Errorf("dnstt: domain %s leaves only %d bytes for payload, need at least %d",
			domain, mtu, minMtu)
	}
	log.Printf("dnstt: effective MTU %d", mtu)

	helloID, err := sampleUTLSDistribution(c.cfg.UTLSDistribution)
	if err != nil {
		return fmt.Errorf("dnstt: parsing uTLS distribution: %w", err)
	}

	if err := c.dialAnyResolver(domain, pubkey, mtu, helloID); err != nil {
		return err
	}

	ns, err := newNetStack(c.cfg.TunFd, c.cfg.TunMtu, c)
	if err != nil {
		c.teardownSession()
		return fmt.Errorf("dnstt: attaching network stack to TUN: %w", err)
	}
	c.netStack = ns

	c.ctx, c.cancel = context.WithCancel(context.Background())
	c.running = true

	// Watch the session: a DNS tunnel loses it whenever the resolver path
	// breaks, and without this the tunnel stays dead behind a connected UI.
	go c.supervise(c.sess, domain, pubkey, mtu, helloID)

	c.notifyState(StateConnected)
	return nil
}

// dialAnyResolver establishes a session over the first usable resolver,
// trying them in weighted-random order. Must be called with c.mu held.
func (c *Client) dialAnyResolver(domain dns.Name, pubkey []byte, mtu int, helloID *utls.ClientHelloID) error {
	var lastErr error
	for _, spec := range orderedResolvers(c.cfg.Resolvers) {
		if err := c.connect(spec, domain, pubkey, mtu, helloID); err != nil {
			log.Printf("dnstt: resolver %s unusable: %v", spec, err)
			c.teardownSession()
			lastErr = err
			continue
		}
		return nil
	}
	if lastErr != nil {
		return fmt.Errorf("dnstt: no usable resolver: %w", lastErr)
	}
	return errors.New("dnstt: no usable resolver")
}

// supervise waits for the session to die and then rebuilds it with backoff.
func (c *Client) supervise(sess *smux.Session, domain dns.Name, pubkey []byte, mtu int, helloID *utls.ClientHelloID) {
	if sess == nil {
		return
	}
	select {
	case <-c.ctx.Done():
		return
	case <-sess.CloseChan():
	}

	c.mu.Lock()
	current := c.running && c.sess == sess
	c.mu.Unlock()
	if !current {
		// Stopped, or already replaced by a newer session.
		return
	}

	log.Printf("dnstt: tunnel session lost, reconnecting")
	c.notifyState(StateReconnecting)

	backoff := initialBackoff
	for attempt := 1; ; attempt++ {
		select {
		case <-c.ctx.Done():
			return
		case <-time.After(backoff):
		}

		c.mu.Lock()
		if !c.running {
			c.mu.Unlock()
			return
		}
		c.teardownSession()
		err := c.dialAnyResolver(domain, pubkey, mtu, helloID)
		newSess := c.sess
		c.mu.Unlock()

		if err == nil && newSess != nil {
			log.Printf("dnstt: tunnel restored after %d attempt(s)", attempt)
			c.notifyState(StateConnected)
			go c.supervise(newSess, domain, pubkey, mtu, helloID)
			return
		}

		log.Printf("dnstt: reconnect attempt %d failed: %v", attempt, err)
		if backoff *= 2; backoff > maxBackoff {
			backoff = maxBackoff
		}
	}
}

// connect establishes the DNS transport, KCP session, Noise channel and smux
// session for a single resolver.
func (c *Client) connect(spec string, domain dns.Name, pubkey []byte, mtu int, helloID *utls.ClientHelloID) error {
	remoteAddr, transport, err := c.buildTransport(spec, helloID)
	if err != nil {
		return err
	}
	c.transport = transport

	pconn := NewDNSPacketConn(transport, remoteAddr, domain)
	c.pconn = pconn

	kcpConn, err := kcp.NewConn2(remoteAddr, nil, 0, 0, pconn)
	if err != nil {
		return fmt.Errorf("opening KCP conn: %w", err)
	}
	c.kcpConn = kcpConn

	// Permit coalescing the payloads of consecutive sends.
	kcpConn.SetStreamMode(true)
	// Disable the dynamic congestion window (limit only by the maximum of
	// local and remote static windows).
	kcpConn.SetNoDelay(
		0, // default nodelay
		0, // default interval
		0, // default resend
		1, // nc=1 => congestion window off
	)
	kcpConn.SetWindowSize(turbotunnel.QueueSize/2, turbotunnel.QueueSize/2)
	if ok := kcpConn.SetMtu(mtu); !ok {
		return fmt.Errorf("could not set KCP MTU to %d", mtu)
	}

	rw, err := c.noiseHandshake(kcpConn, pubkey)
	if err != nil {
		return err
	}

	smuxConfig := smux.DefaultConfig()
	smuxConfig.Version = 2
	smuxConfig.KeepAliveTimeout = idleTimeout
	smuxConfig.MaxStreamBuffer = 1 * 1024 * 1024 // default is 65536
	sess, err := smux.Client(rw, smuxConfig)
	if err != nil {
		return fmt.Errorf("opening smux session: %w", err)
	}
	c.sess = sess
	c.liveSession.Store(sess)

	log.Printf("dnstt: session %08x established via %s", kcpConn.GetConv(), spec)
	return nil
}

// noiseHandshake runs the Noise NK handshake under a timeout. Without this a
// silently blackholing resolver would block Start forever.
func (c *Client) noiseHandshake(kcpConn *kcp.UDPSession, pubkey []byte) (rwc interface {
	Read([]byte) (int, error)
	Write([]byte) (int, error)
	Close() error
}, err error) {
	type result struct {
		rw  interface {
			Read([]byte) (int, error)
			Write([]byte) (int, error)
			Close() error
		}
		err error
	}
	ch := make(chan result, 1)
	go func() {
		rw, err := noise.NewClient(kcpConn, pubkey)
		ch <- result{rw, err}
	}()

	select {
	case r := <-ch:
		if r.err != nil {
			return nil, fmt.Errorf("noise handshake: %w", r.err)
		}
		return r.rw, nil
	case <-time.After(handshakeTimeout):
		// Unblock the goroutine parked in the handshake.
		kcpConn.Close()
		return nil, fmt.Errorf("noise handshake timed out after %s", handshakeTimeout)
	}
}

// openStream opens a new multiplexed stream to the dnstt server. While the
// session is dead it fails fast with errTunnelDown so that callers can stay
// quiet until the supervisor rebuilds the tunnel.
func (c *Client) openStream() (*smux.Stream, error) {
	sess := c.liveSession.Load()
	if sess == nil || sess.IsClosed() {
		return nil, errTunnelDown
	}
	stream, err := sess.OpenStream()
	if err != nil {
		if errors.Is(err, io.ErrClosedPipe) {
			return nil, errTunnelDown
		}
		return nil, err
	}
	return stream, nil
}

// isTunnelDown reports whether err means "no live session" rather than a real
// per-stream failure.
func isTunnelDown(err error) bool {
	return errors.Is(err, errTunnelDown) || errors.Is(err, io.ErrClosedPipe)
}

// teardownSession closes the transport chain, leaving the TUN stack alone.
func (c *Client) teardownSession() {
	c.liveSession.Store(nil)
	if c.sess != nil {
		c.sess.Close()
		c.sess = nil
	}
	if c.kcpConn != nil {
		c.kcpConn.Close()
		c.kcpConn = nil
	}
	if c.pconn != nil {
		c.pconn.Close()
		c.pconn = nil
	}
	if c.transport != nil {
		c.transport.Close()
		c.transport = nil
	}
}

// Stop tears the tunnel down. It is safe to call more than once.
func (c *Client) Stop() error {
	c.mu.Lock()
	defer c.mu.Unlock()

	if !c.running {
		c.teardownSession()
		return nil
	}
	c.running = false
	if c.cancel != nil {
		c.cancel()
	}

	if c.netStack != nil {
		c.netStack.Close()
		c.netStack = nil
	}
	c.teardownSession()
	log.Printf("dnstt: tunnel stopped")
	c.notifyState(StateDisconnected)
	return nil
}

// sampleUTLSDistribution parses a weighted uTLS Client Hello ID distribution
// string of the form "3*Firefox,2*Chrome,1*iOS" and samples one ID from it.
// The label "none" disables uTLS. Adapted from dnstt-client.
func sampleUTLSDistribution(spec string) (*utls.ClientHelloID, error) {
	weights, labels, err := parseWeightedList(spec)
	if err != nil {
		return nil, err
	}
	ids := make([]*utls.ClientHelloID, 0, len(labels))
	for _, label := range labels {
		var id *utls.ClientHelloID
		if label != "none" {
			id = utlsLookup(label)
			if id == nil {
				return nil, fmt.Errorf("unknown TLS fingerprint %q", label)
			}
		}
		ids = append(ids, id)
	}
	if len(ids) == 0 {
		return nil, errors.New("empty uTLS distribution")
	}
	return ids[sampleWeighted(weights)], nil
}

// parseDoHURL validates a DoH endpoint.
func parseDoHURL(spec string) error {
	u, err := url.Parse(spec)
	if err != nil {
		return err
	}
	if u.Scheme != "https" || u.Host == "" {
		return fmt.Errorf("dnstt: bad DoH URL %q", spec)
	}
	return nil
}
