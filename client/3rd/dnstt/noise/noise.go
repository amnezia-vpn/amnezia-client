// Package noise provides a net.Conn-like interface for a
// Noise_NK_25519_ChaChaPoly_BLAKE2s. It encodes Noise messages onto a reliable
// stream using 16-bit length prefixes.
//
// https://noiseprotocol.org/noise.html
package noise

import (
	"bufio"
	"crypto/rand"
	"encoding/binary"
	"encoding/hex"
	"errors"
	"fmt"
	"io"
	"strings"

	"github.com/flynn/noise"
	"golang.org/x/crypto/curve25519"
)

// The length of public and private keys as returned by GeneratePrivkey.
const KeyLen = 32

// cipherSuite represents 25519_ChaChaPoly_BLAKE2s.
var cipherSuite = noise.NewCipherSuite(noise.DH25519, noise.CipherChaChaPoly, noise.HashBLAKE2s)

// readMessage reads a length-prefixed message from r. It returns a nil error
// only when a complete message was read. It returns io.EOF only when there were
// 0 bytes remaining to read from r. It returns io.ErrUnexpectedEOF when EOF
// occurs in the middle of an encoded message.
func readMessage(r io.Reader) ([]byte, error) {
	var length uint16
	err := binary.Read(r, binary.BigEndian, &length)
	if err != nil {
		// We may return a real io.EOF only here.
		return nil, err
	}
	msg := make([]byte, int(length))
	_, err = io.ReadFull(r, msg)
	// Here we must change io.EOF to io.ErrUnexpectedEOF.
	if err == io.EOF {
		err = io.ErrUnexpectedEOF
	}
	return msg, err
}

// writeMessage writes msg as a length-prefixed message to w. It panics if the
// length of msg cannot be represented in 16 bits.
func writeMessage(w io.Writer, msg []byte) error {
	length := uint16(len(msg))
	if int(length) != len(msg) {
		panic(len(msg))
	}
	err := binary.Write(w, binary.BigEndian, length)
	if err != nil {
		return err
	}
	_, err = w.Write(msg)
	return err
}

// socket is the internal type that represents a Noise-wrapped
// io.ReadWriteCloser.
type socket struct {
	recvPipe   *io.PipeReader
	sendCipher *noise.CipherState
	io.ReadWriteCloser
}

func newSocket(rwc io.ReadWriteCloser, recvCipher, sendCipher *noise.CipherState) *socket {
	pr, pw := io.Pipe()
	// This loop calls readMessage, decrypts the messages, and feeds them
	// into recvPipe where they will be returned from Read.
	go func() (err error) {
		defer func() {
			pw.CloseWithError(err)
		}()
		for {
			msg, err := readMessage(rwc)
			if err != nil {
				return err
			}
			p, err := recvCipher.Decrypt(nil, nil, msg)
			if err != nil {
				return err
			}
			_, err = pw.Write(p)
			if err != nil {
				return err
			}
		}
	}()
	return &socket{
		sendCipher:      sendCipher,
		recvPipe:        pr,
		ReadWriteCloser: rwc,
	}
}

// Read reads decrypted data from the wrapped io.Reader.
func (s *socket) Read(p []byte) (int, error) {
	return s.recvPipe.Read(p)
}

// Write writes encrypted data from the wrapped io.Writer.
func (s *socket) Write(p []byte) (int, error) {
	total := 0
	for len(p) > 0 {
		n := len(p)
		if n > 4096 {
			n = 4096
		}
		msg, err := s.sendCipher.Encrypt(nil, nil, p[:n])
		if err != nil {
			return total, err
		}
		err = writeMessage(s.ReadWriteCloser, msg)
		if err != nil {
			return total, err
		}
		total += n
		p = p[n:]
	}
	return total, nil
}

// newConfig instantiates configuration settings that are common to clients and
// servers.
func newConfig() noise.Config {
	return noise.Config{
		CipherSuite: cipherSuite,
		Pattern:     noise.HandshakeNK,
		Prologue:    []byte("dnstt 2020-04-13"),
	}
}

// NewClient wraps an io.ReadWriteCloser in a Noise protocol as a client, and
// returns after completing the handshake. It returns a non-nil error if there
// is an error during the handshake.
func NewClient(rwc io.ReadWriteCloser, serverPubkey []byte) (io.ReadWriteCloser, error) {
	config := newConfig()
	config.Initiator = true
	config.PeerStatic = serverPubkey
	handshakeState, err := noise.NewHandshakeState(config)
	if err != nil {
		return nil, err
	}

	// -> e, es
	msg, _, _, err := handshakeState.WriteMessage(nil, nil)
	if err != nil {
		return nil, err
	}
	err = writeMessage(rwc, msg)
	if err != nil {
		return nil, err
	}

	// <- e, es
	msg, err = readMessage(rwc)
	if err != nil {
		return nil, err
	}
	payload, sendCipher, recvCipher, err := handshakeState.ReadMessage(nil, msg)
	if err != nil {
		return nil, err
	}
	if len(payload) != 0 {
		return nil, errors.New("unexpected server payload")
	}

	return newSocket(rwc, recvCipher, sendCipher), nil
}

// NewClient wraps an io.ReadWriteCloser in a Noise protocol as a server, and
// returns after completing the handshake. It returns a non-nil error if there
// is an error during the handshake.
func NewServer(rwc io.ReadWriteCloser, serverPrivkey []byte) (io.ReadWriteCloser, error) {
	config := newConfig()
	config.Initiator = false
	config.StaticKeypair = noise.DHKey{
		Private: serverPrivkey,
		Public:  PubkeyFromPrivkey(serverPrivkey),
	}
	handshakeState, err := noise.NewHandshakeState(config)
	if err != nil {
		return nil, err
	}

	// -> e, es
	msg, err := readMessage(rwc)
	if err != nil {
		return nil, err
	}
	payload, _, _, err := handshakeState.ReadMessage(nil, msg)
	if err != nil {
		return nil, err
	}
	if len(payload) != 0 {
		return nil, errors.New("unexpected client payload")
	}

	// <- e, es
	msg, recvCipher, sendCipher, err := handshakeState.WriteMessage(nil, nil)
	if err != nil {
		return nil, err
	}
	err = writeMessage(rwc, msg)
	if err != nil {
		return nil, err
	}

	return newSocket(rwc, recvCipher, sendCipher), nil
}

// GeneratePrivkey generates a private key. The corresponding public key can be
// derived using PubkeyFromPrivkey.
func GeneratePrivkey() ([]byte, error) {
	pair, err := noise.DH25519.GenerateKeypair(rand.Reader)
	return pair.Private, err
}

// PubkeyFromPrivkey returns the public key that corresponds to privkey.
func PubkeyFromPrivkey(privkey []byte) []byte {
	pubkey, err := curve25519.X25519(privkey, curve25519.Basepoint)
	if err != nil {
		panic(err)
	}
	return pubkey
}

// ReadKey reads a hex-encoded key from r. r must consist of a single line, with
// or without a '\n' line terminator. The line must consist of KeyLen
// hex-encoded bytes.
func ReadKey(r io.Reader) ([]byte, error) {
	br := bufio.NewReader(io.LimitReader(r, 100))
	line, err := br.ReadString('\n')
	if err == io.EOF {
		err = nil
	}
	if err == nil {
		// Check that we're at EOF.
		_, err = br.ReadByte()
		if err == io.EOF {
			err = nil
		} else if err == nil {
			err = fmt.Errorf("file contains more than one line")
		}
	}
	if err != nil {
		return nil, err
	}
	line = strings.TrimSuffix(line, "\n")
	return DecodeKey(line)
}

// WriteKey writes the hex-encoded key in a single line to w.
func WriteKey(w io.Writer, key []byte) error {
	_, err := fmt.Fprintf(w, "%x\n", key)
	return err
}

// DecodeKey decodes a hex-encoded private or public key.
func DecodeKey(s string) ([]byte, error) {
	key, err := hex.DecodeString(s)
	if err == nil && len(key) != KeyLen {
		err = fmt.Errorf("length is %d, expected %d", len(key), KeyLen)
	}
	return key, err
}

// EncodeKey encodes a hex-encoded private or public key.
func EncodeKey(key []byte) string {
	return hex.EncodeToString(key)
}
