package dnsttclient

// gVisor network stack attached to the VpnService TUN. Packets from the TUN are
// terminated locally and re-originated as smux streams through the dnstt
// tunnel, each stream carrying a SOCKS5 session to the proxy behind the server.

import (
	"encoding/binary"
	"errors"
	"fmt"
	"io"
	"log"
	"net"
	"strconv"
	"sync"
	"time"

	"github.com/xjasonlyu/tun2socks/v2/core"
	"github.com/xjasonlyu/tun2socks/v2/core/adapter"
	"github.com/xjasonlyu/tun2socks/v2/core/device"
	"github.com/xjasonlyu/tun2socks/v2/core/device/fdbased"
	gstack "gvisor.dev/gvisor/pkg/tcpip/stack"
)

const (
	// dnsPort is the only UDP port the tunnel can serve. dnstt carries TCP
	// streams only, so DNS is relayed as DNS-over-TCP (RFC 1035 §4.2.2).
	dnsPort = 53

	// dnsQueryTimeout bounds one relayed DNS exchange.
	dnsQueryTimeout = 15 * time.Second

	// udpSessionIdle closes a UDP endpoint that has gone quiet.
	udpSessionIdle = 60 * time.Second

	// maxDNSMessage is the largest DNS message accepted in either direction.
	maxDNSMessage = 65535
)

// netStack owns the gVisor stack and the TUN device backing it.
type netStack struct {
	client *Client
	dev    device.Device
	stack  *gstack.Stack

	closeOnce sync.Once
}

// newNetStack attaches a gVisor stack to tunFd. The descriptor is owned by the
// stack from this point on and is closed by Close.
func newNetStack(tunFd, mtu int, client *Client) (*netStack, error) {
	if tunFd <= 0 {
		return nil, fmt.Errorf("invalid TUN descriptor %d", tunFd)
	}
	if mtu <= 0 {
		mtu = 1500
	}

	dev, err := fdbased.Open(strconv.Itoa(tunFd), uint32(mtu), 0)
	if err != nil {
		return nil, fmt.Errorf("opening TUN device: %w", err)
	}

	ns := &netStack{client: client, dev: dev}

	st, err := core.CreateStack(&core.Config{
		LinkEndpoint:     dev,
		TransportHandler: ns,
	})
	if err != nil {
		dev.Close()
		return nil, fmt.Errorf("creating network stack: %w", err)
	}
	ns.stack = st

	log.Printf("dnstt: network stack attached to TUN (mtu %d)", mtu)
	return ns, nil
}

// Close shuts the stack down and releases the TUN descriptor.
func (ns *netStack) Close() {
	ns.closeOnce.Do(func() {
		if ns.stack != nil {
			ns.stack.Close()
		}
		if ns.dev != nil {
			ns.dev.Close()
		}
	})
}

// destination returns the address the application was trying to reach.
func destination(id gstack.TransportEndpointID) (net.IP, uint16) {
	return net.IP(id.LocalAddress.AsSlice()), id.LocalPort
}

// HandleTCP implements adapter.TransportHandler.
func (ns *netStack) HandleTCP(conn adapter.TCPConn) {
	go ns.handleTCP(conn)
}

// HandleUDP implements adapter.TransportHandler.
func (ns *netStack) HandleUDP(conn adapter.UDPConn) {
	go ns.handleUDP(conn)
}

func (ns *netStack) handleTCP(conn adapter.TCPConn) {
	defer conn.Close()

	dstIP, dstPort := destination(conn.ID())

	stream, err := ns.client.openStream()
	if err != nil {
		dst := net.JoinHostPort(dstIP.String(), strconv.Itoa(int(dstPort)))
		if isTunnelDown(err) {
			// The supervisor is rebuilding the session; drop the flow
			// quietly rather than logging every connection attempt.
			ns.client.logTunnelDown("dropping TCP connection to "+dst, err)
		} else {
			log.Printf("dnstt: opening stream for %s: %v", dst, err)
		}
		return
	}
	defer stream.Close()

	if err := socks5Connect(stream, dstIP, dstPort); err != nil {
		log.Printf("dnstt: socks5 connect to %s: %v", net.JoinHostPort(dstIP.String(), strconv.Itoa(int(dstPort))), err)
		return
	}

	var wg sync.WaitGroup
	wg.Add(2)
	go func() {
		defer wg.Done()
		if _, err := io.Copy(stream, conn); err != nil && !isExpectedCopyError(err) {
			log.Printf("dnstt: copy stream←tun: %v", err)
		}
		stream.Close()
	}()
	go func() {
		defer wg.Done()
		if _, err := io.Copy(conn, stream); err != nil && !isExpectedCopyError(err) {
			log.Printf("dnstt: copy tun←stream: %v", err)
		}
		conn.Close()
	}()
	wg.Wait()
}

// isExpectedCopyError reports whether err is the ordinary end of a proxied
// stream rather than something worth logging.
func isExpectedCopyError(err error) bool {
	return err == nil ||
		errors.Is(err, io.EOF) ||
		errors.Is(err, io.ErrClosedPipe) ||
		errors.Is(err, net.ErrClosed)
}

func (ns *netStack) handleUDP(conn adapter.UDPConn) {
	defer conn.Close()

	dstIP, dstPort := destination(conn.ID())
	if dstPort != dnsPort {
		// dnstt is a TCP-only tunnel: there is nothing correct we can do
		// with general UDP, so the flow is dropped rather than leaked
		// outside the tunnel.
		return
	}

	buf := make([]byte, 4096)
	for {
		if err := conn.SetReadDeadline(time.Now().Add(udpSessionIdle)); err != nil {
			return
		}
		n, _, err := conn.ReadFrom(buf)
		if err != nil {
			return
		}

		query := make([]byte, n)
		copy(query, buf[:n])

		go func() {
			resp, err := ns.resolveOverTunnel(dstIP, dstPort, query)
			if err != nil {
				if isTunnelDown(err) {
					ns.client.logTunnelDown("dropping DNS query to "+dstIP.String(), err)
				} else {
					log.Printf("dnstt: relaying DNS query to %s: %v", dstIP, err)
				}
				return
			}
			// A nil address writes back through the endpoint to the
			// application that sent the query, which is how tun2socks
			// replies on a gVisor UDP conn.
			if _, err := conn.WriteTo(resp, nil); err != nil {
				log.Printf("dnstt: writing DNS reply: %v", err)
			}
		}()
	}
}

// resolveOverTunnel relays a single UDP DNS query as DNS-over-TCP through the
// tunnel and returns the response payload.
func (ns *netStack) resolveOverTunnel(dstIP net.IP, dstPort uint16, query []byte) ([]byte, error) {
	if len(query) > maxDNSMessage {
		return nil, fmt.Errorf("DNS query of %d bytes is too large", len(query))
	}

	stream, err := ns.client.openStream()
	if err != nil {
		return nil, err
	}
	defer stream.Close()

	if err := stream.SetDeadline(time.Now().Add(dnsQueryTimeout)); err != nil {
		return nil, err
	}
	if err := socks5Connect(stream, dstIP, dstPort); err != nil {
		return nil, err
	}

	framed := binary.BigEndian.AppendUint16(make([]byte, 0, len(query)+2), uint16(len(query)))
	framed = append(framed, query...)
	if _, err := stream.Write(framed); err != nil {
		return nil, fmt.Errorf("writing DNS query: %w", err)
	}

	var lengthPrefix [2]byte
	if _, err := io.ReadFull(stream, lengthPrefix[:]); err != nil {
		return nil, fmt.Errorf("reading DNS reply length: %w", err)
	}
	respLen := int(binary.BigEndian.Uint16(lengthPrefix[:]))
	resp := make([]byte, respLen)
	if _, err := io.ReadFull(stream, resp); err != nil {
		return nil, fmt.Errorf("reading DNS reply: %w", err)
	}
	return resp, nil
}
