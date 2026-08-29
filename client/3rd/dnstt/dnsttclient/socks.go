package dnsttclient

// Minimal SOCKS5 client, spoken over an smux stream. The dnstt server forwards
// every stream to one fixed upstream address; Amnezia expects that address to
// be a SOCKS5 proxy, which is what makes arbitrary destinations reachable
// through a tunnel that itself carries no addressing information.

import (
	"encoding/binary"
	"fmt"
	"io"
	"net"
)

const (
	socks5Version = 0x05

	socksAuthNone        = 0x00
	socksAuthUnavailable = 0xff

	socksCmdConnect = 0x01

	socksAtypIPv4 = 0x01
	socksAtypIPv6 = 0x04
)

// socksReplyMessage maps a SOCKS5 reply code to a readable error.
func socksReplyMessage(code byte) string {
	switch code {
	case 0x01:
		return "general SOCKS server failure"
	case 0x02:
		return "connection not allowed by ruleset"
	case 0x03:
		return "network unreachable"
	case 0x04:
		return "host unreachable"
	case 0x05:
		return "connection refused"
	case 0x06:
		return "TTL expired"
	case 0x07:
		return "command not supported"
	case 0x08:
		return "address type not supported"
	default:
		return fmt.Sprintf("unknown reply code 0x%02x", code)
	}
}

// socks5Connect performs a SOCKS5 CONNECT handshake for dst over rw. Only the
// no-authentication method is offered, matching the loopback SOCKS5 proxies
// dnstt servers are pointed at.
func socks5Connect(rw io.ReadWriter, dstIP net.IP, dstPort uint16) error {
	// Greeting: version, one method, no authentication.
	if _, err := rw.Write([]byte{socks5Version, 0x01, socksAuthNone}); err != nil {
		return fmt.Errorf("socks5 greeting: %w", err)
	}

	var methodReply [2]byte
	if _, err := io.ReadFull(rw, methodReply[:]); err != nil {
		return fmt.Errorf("socks5 method reply: %w", err)
	}
	if methodReply[0] != socks5Version {
		return fmt.Errorf("socks5: unexpected version 0x%02x", methodReply[0])
	}
	if methodReply[1] == socksAuthUnavailable {
		return fmt.Errorf("socks5: proxy rejected the no-auth method")
	}
	if methodReply[1] != socksAuthNone {
		return fmt.Errorf("socks5: proxy selected unsupported auth method 0x%02x", methodReply[1])
	}

	// Request: CONNECT to a literal address; the netstack always hands us an
	// IP, so no name resolution happens on this side.
	req := []byte{socks5Version, socksCmdConnect, 0x00}
	if ip4 := dstIP.To4(); ip4 != nil {
		req = append(req, socksAtypIPv4)
		req = append(req, ip4...)
	} else if ip16 := dstIP.To16(); ip16 != nil {
		req = append(req, socksAtypIPv6)
		req = append(req, ip16...)
	} else {
		return fmt.Errorf("socks5: unsupported destination address %v", dstIP)
	}
	req = binary.BigEndian.AppendUint16(req, dstPort)
	if _, err := rw.Write(req); err != nil {
		return fmt.Errorf("socks5 connect request: %w", err)
	}

	// Reply: version, status, reserved, bound address.
	var head [4]byte
	if _, err := io.ReadFull(rw, head[:]); err != nil {
		return fmt.Errorf("socks5 connect reply: %w", err)
	}
	if head[0] != socks5Version {
		return fmt.Errorf("socks5: unexpected version 0x%02x in reply", head[0])
	}
	if head[1] != 0x00 {
		return fmt.Errorf("socks5: %s", socksReplyMessage(head[1]))
	}

	// Drain the bound address so the stream is positioned at the payload.
	var skip int
	switch head[3] {
	case socksAtypIPv4:
		skip = net.IPv4len + 2
	case socksAtypIPv6:
		skip = net.IPv6len + 2
	case 0x03: // domain name
		var l [1]byte
		if _, err := io.ReadFull(rw, l[:]); err != nil {
			return fmt.Errorf("socks5 bound address length: %w", err)
		}
		skip = int(l[0]) + 2
	default:
		return fmt.Errorf("socks5: unsupported bound address type 0x%02x", head[3])
	}
	if _, err := io.CopyN(io.Discard, rw, int64(skip)); err != nil {
		return fmt.Errorf("socks5 bound address: %w", err)
	}

	return nil
}
