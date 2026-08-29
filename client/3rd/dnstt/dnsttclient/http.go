package dnsttclient

import (
	"bytes"
	"fmt"
	"io"
	"io/ioutil"
	"log"
	"net/http"
	"strconv"
	"sync"
	"time"

	"org.amnezia.vpn/dnstt/turbotunnel"
)

// A default Retry-After delay to use when there is no explicit Retry-After
// header in an HTTP response.
const defaultRetryAfter = 10 * time.Second

// HTTPPacketConn is an HTTP-based transport for DNS messages, used for DNS over
// HTTPS (DoH). Its WriteTo and ReadFrom methods exchange DNS messages over HTTP
// requests and responses.
//
// HTTPPacketConn deals only with already formatted DNS messages. It does not
// handle encoding information into the messages. That is rather the
// responsibility of DNSPacketConn.
//
// https://tools.ietf.org/html/rfc8484
type HTTPPacketConn struct {
	// client is the http.Client used to make requests. We use this instead
	// of http.DefaultClient in order to support setting a timeout and a
	// uTLS fingerprint.
	client *http.Client

	// urlString is the URL to which HTTP requests will be sent, for example
	// "https://doh.example/dns-query".
	urlString string

	// notBefore, if not zero, is a time before which we may not send any
	// queries; queries are buffered or dropped until that time. notBefore
	// is set when we get a 429 Too Many Requests HTTP response or other
	// unexpected status code that causes us to need to slow down. It is set
	// according to the Retry-After header if available, otherwise it is set
	// to defaultRetryAfter in the future. notBeforeLock controls access to
	// notBefore.
	notBefore     time.Time
	notBeforeLock sync.RWMutex

	// QueuePacketConn is the direct receiver of ReadFrom and WriteTo calls.
	// sendLoop, via send, removes messages from the outgoing queue that
	// were placed there by WriteTo, and inserts messages into the incoming
	// queue to be returned from ReadFrom.
	*turbotunnel.QueuePacketConn
}

// NewHTTPPacketConn creates a new HTTPPacketConn configured to use the HTTP
// server at urlString as a DNS over HTTP resolver. client is the http.Client
// that will be used to make requests. urlString should include any necessary
// path components; e.g., "/dns-query". numSenders is the number of concurrent
// sender-receiver goroutines to run.
func NewHTTPPacketConn(rt http.RoundTripper, urlString string, numSenders int) (*HTTPPacketConn, error) {
	c := &HTTPPacketConn{
		client: &http.Client{
			Transport: rt,
			Timeout:   1 * time.Minute,
		},
		urlString:       urlString,
		QueuePacketConn: turbotunnel.NewQueuePacketConn(turbotunnel.DummyAddr{}, 0),
	}
	for i := 0; i < numSenders; i++ {
		go c.sendLoop()
	}
	return c, nil
}

// send sends a message in an HTTP request, and queues the body HTTP response to
// be returned from a future call to ReadFrom.
func (c *HTTPPacketConn) send(p []byte) error {
	req, err := http.NewRequest("POST", c.urlString, bytes.NewReader(p))
	if err != nil {
		return err
	}
	req.Header.Set("Accept", "application/dns-message")
	req.Header.Set("Content-Type", "application/dns-message")
	req.Header.Set("User-Agent", "") // Disable default "Go-http-client/1.1".
	resp, err := c.client.Do(req)
	if err != nil {
		return err
	}
	defer resp.Body.Close()

	switch resp.StatusCode {
	case http.StatusOK:
		if ct := resp.Header.Get("Content-Type"); ct != "application/dns-message" {
			return fmt.Errorf("unknown HTTP response Content-Type %+q", ct)
		}
		body, err := ioutil.ReadAll(io.LimitReader(resp.Body, 64000))
		if err == nil {
			c.QueuePacketConn.QueueIncoming(body, turbotunnel.DummyAddr{})
		}
		// Ignore err != nil; don't report an error if we at least
		// managed to send.
	default:
		// We primarily are thinking of 429 Too Many Requests here, but
		// any other unexpected response codes will also cause us to
		// rate-limit ourselves and emit a log message.
		// https://developers.google.com/speed/public-dns/docs/doh/#errors
		now := time.Now()
		var retryAfter time.Time
		if value := resp.Header.Get("Retry-After"); value != "" {
			var err error
			retryAfter, err = parseRetryAfter(value, now)
			if err != nil {
				log.Printf("cannot parse Retry-After value %+q", value)
			}
		}
		if retryAfter.IsZero() {
			// Supply a default.
			retryAfter = now.Add(defaultRetryAfter)
		}
		if retryAfter.Before(now) {
			log.Printf("got %+q, but Retry-After is %v in the past",
				resp.Status, now.Sub(retryAfter))
		} else {
			c.notBeforeLock.Lock()
			if retryAfter.Before(c.notBefore) {
				log.Printf("got %+q, but Retry-After is %v earlier than already received Retry-After",
					resp.Status, c.notBefore.Sub(retryAfter))
			} else {
				log.Printf("got %+q; ceasing sending for %v",
					resp.Status, retryAfter.Sub(now))
				c.notBefore = retryAfter
			}
			c.notBeforeLock.Unlock()
		}
	}

	return nil
}

// sendLoop loops over the contents of the outgoing queue and passes them to
// send. It drops packets while c.notBefore is in the future.
func (c *HTTPPacketConn) sendLoop() {
	for p := range c.QueuePacketConn.OutgoingQueue(turbotunnel.DummyAddr{}) {
		// Stop sending while we are rate-limiting ourselves (as a
		// result of a Retry-After response header, for example).
		c.notBeforeLock.RLock()
		notBefore := c.notBefore
		c.notBeforeLock.RUnlock()
		if wait := notBefore.Sub(time.Now()); wait > 0 {
			// Drop it.
			continue
		}

		err := c.send(p)
		if err != nil {
			log.Printf("sendLoop: %v", err)
		}
	}
}

// parseRetryAfter parses the value of a Retry-After header as an absolute
// time.Time.
func parseRetryAfter(value string, now time.Time) (time.Time, error) {
	// May be a date string or an integer number of seconds.
	// https://tools.ietf.org/html/rfc7231#section-7.1.3
	if t, err := http.ParseTime(value); err == nil {
		return t, nil
	}
	i, err := strconv.ParseUint(value, 10, 32)
	if err != nil {
		return time.Time{}, err
	}
	return now.Add(time.Duration(i) * time.Second), nil
}
