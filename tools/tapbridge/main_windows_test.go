package main

import (
	"bytes"
	"encoding/binary"
	"errors"
	"io"
	"net/http"
	"strings"
	"testing"
)

type roundTripFunc func(*http.Request) (*http.Response, error)

func (f roundTripFunc) RoundTrip(request *http.Request) (*http.Response, error) {
	return f(request)
}

func TestForwardDNSUsesDoHAndReturnsBinaryAnswer(t *testing.T) {
	query := []byte{0x12, 0x34, 0x01, 0x00, 0x00, 0x01}
	answer := []byte{0x12, 0x34, 0x81, 0x80, 0x00, 0x01}
	client := &http.Client{Transport: roundTripFunc(func(request *http.Request) (*http.Response, error) {
		body, err := io.ReadAll(request.Body)
		if err != nil {
			return nil, err
		}
		if request.Method != http.MethodPost || request.URL.String() != "https://1.1.1.1/dns-query" {
			t.Errorf("unexpected DoH request: %s %s", request.Method, request.URL)
		}
		if request.Host != "cloudflare-dns.com" {
			t.Errorf("unexpected DoH host: %q", request.Host)
		}
		if request.Header.Get("Content-Type") != "application/dns-message" || request.Header.Get("Accept") != "application/dns-message" {
			t.Errorf("unexpected DoH headers: %v", request.Header)
		}
		if !bytes.Equal(body, query) {
			t.Errorf("DoH request body %x, want %x", body, query)
		}
		return &http.Response{
			StatusCode: http.StatusOK,
			Status:     "200 OK",
			Body:       io.NopCloser(bytes.NewReader(answer)),
			Header:     make(http.Header),
			Request:    request,
		}, nil
	})}

	got, err := (&shareHandler{doh: client}).forwardDNS(query)
	if err != nil {
		t.Fatal(err)
	}
	if !bytes.Equal(got, answer) {
		t.Fatalf("DNS answer %x, want %x", got, answer)
	}
}

func TestForwardDNSFallsBackToGoogleDoH(t *testing.T) {
	answer := []byte{0x12, 0x34, 0x81, 0x80}
	primary := &http.Client{Transport: roundTripFunc(func(*http.Request) (*http.Response, error) {
		return nil, errors.New("Cloudflare unavailable")
	})}
	fallback := &http.Client{Transport: roundTripFunc(func(request *http.Request) (*http.Response, error) {
		if request.URL.String() != "https://8.8.8.8/dns-query" || request.Host != "dns.google" {
			t.Errorf("unexpected fallback DoH request: %s host=%q", request.URL, request.Host)
		}
		return &http.Response{
			StatusCode: http.StatusOK,
			Status:     "200 OK",
			Body:       io.NopCloser(bytes.NewReader(answer)),
			Header:     make(http.Header),
			Request:    request,
		}, nil
	})}
	got, err := (&shareHandler{doh: primary, dohFallback: fallback}).forwardDNS([]byte{0x12, 0x34, 0, 0})
	if err != nil {
		t.Fatal(err)
	}
	if !bytes.Equal(got, answer) {
		t.Fatalf("fallback DNS answer %x, want %x", got, answer)
	}
}

func TestForwardDNSRejectsOversizedAnswers(t *testing.T) {
	client := &http.Client{Transport: roundTripFunc(func(request *http.Request) (*http.Response, error) {
		return &http.Response{
			StatusCode: http.StatusOK,
			Status:     "200 OK",
			Body:       io.NopCloser(strings.NewReader(strings.Repeat("x", 4097))),
			Header:     make(http.Header),
			Request:    request,
		}, nil
	})}
	if _, err := (&shareHandler{doh: client}).forwardDNS([]byte{0, 1}); err == nil {
		t.Fatal("expected oversized response to be rejected")
	}
}

func TestIPv4OnlyAAAAResponseReturnsNODATA(t *testing.T) {
	query := []byte{
		0x12, 0x34, 0x01, 0x10, // ID, recursion desired, checking disabled
		0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, // one question and OPT
		0x07, 'e', 'x', 'a', 'm', 'p', 'l', 'e',
		0x03, 'c', 'o', 'm', 0x00, 0x00, 0x1c, 0x00, 0x01,
		0x00, 0x00, 0x29, 0x04, 0xd0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	}
	response, ok := ipv4OnlyAAAAResponse(query)
	if !ok {
		t.Fatal("AAAA query was not recognized")
	}
	if len(response) != 29 || response[0] != 0x12 || response[1] != 0x34 {
		t.Fatalf("unexpected response length or ID: %x", response)
	}
	if response[2] != 0x81 || response[3] != 0x90 {
		t.Fatalf("unexpected DNS response flags: %02x%02x", response[2], response[3])
	}
	if binary.BigEndian.Uint16(response[4:6]) != 1 || binary.BigEndian.Uint16(response[6:8]) != 0 {
		t.Fatalf("expected one question and no answers: %x", response[4:8])
	}
	if !bytes.Equal(response[12:], query[12:29]) {
		t.Fatalf("response did not preserve the AAAA question: %x", response[12:])
	}
}

func TestIPv4OnlyAAAAResponseLeavesOtherQuestionsAlone(t *testing.T) {
	query := []byte{
		0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0,
		0x07, 'e', 'x', 'a', 'm', 'p', 'l', 'e',
		0x03, 'c', 'o', 'm', 0, 0, 1, 0, 1,
	}
	if _, ok := ipv4OnlyAAAAResponse(query); ok {
		t.Fatal("A query must continue through DoH")
	}
	if _, ok := ipv4OnlyAAAAResponse([]byte{1, 2, 3}); ok {
		t.Fatal("malformed query must continue through DoH validation")
	}
}
