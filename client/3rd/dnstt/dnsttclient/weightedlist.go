package dnsttclient

// Random selection from weighted distributions, and strings for specifying such
// distributions.

import (
	cryptorand "crypto/rand"
	"encoding/binary"
	"fmt"
	mathrand "math/rand"
	"strconv"
	"strings"
)

// parseWeightedList parses a list of text labels with optional numeric weights,
// and returns parallel slices of weights and labels. If a weight is omitted for
// a label, the weight is 1.
//
// An example weighted list string is "2*apple,orange,10*cookie". This example
// results in the slices [2, 1, 10] and ["apple", "orange", "cookie"].
// Bytes may be escaped by backslashes.
//
//	list ::= entry ("," entry)*
//	entry ::= (weight "*")? label
func parseWeightedList(s string) ([]uint32, []string, error) {
	const (
		kindEOF = iota
		kindComma
		kindAsterisk
		kindText
		kindError
	)
	type token struct {
		Kind int
		Text string
	}

	var i int
	// nextToken incrementally consumes s and returns tokens.
	nextToken := func() token {
		if !(i < len(s)) {
			return token{Kind: kindEOF}
		}
		if s[i] == ',' {
			i++
			return token{Kind: kindComma}
		}
		if s[i] == '*' {
			i++
			return token{Kind: kindAsterisk}
		}
		var text strings.Builder
		for i < len(s) && s[i] != ',' && s[i] != '*' {
			if s[i] == '\\' {
				i++
				if !(i < len(s)) {
					return token{Kind: kindError, Text: fmt.Sprintf("%q at end of string", s[i])}
				}
			}
			text.WriteByte(s[i])
			i++
		}
		return token{Kind: kindText, Text: text.String()}
	}
	peekToken := func() token {
		saved := i
		t := nextToken()
		i = saved
		return t
	}

	const (
		stateBeginEntry = iota
		stateLabel
		stateEndEntry
		stateDone
		stateUnexpected
	)

	var weights []uint32
	var labels []string
	var weightString, label string
	var t token
	for state := stateBeginEntry; state != stateDone; {
		switch state {
		// Beginning of a new entry (at the beginning of the input or
		// after a comma).
		case stateBeginEntry:
			t = nextToken()
			switch t.Kind {
			case kindText:
				// If the next token is an asterisk, this text
				// represents a weight; otherwise it represents
				// a label (with a weight of "1").
				switch peekToken().Kind {
				case kindAsterisk:
					nextToken() // Consume the asterisk token.
					weightString = t.Text
					state = stateLabel
				default:
					weightString = "1"
					label = t.Text
					state = stateEndEntry
				}
			default:
				state = stateUnexpected
			}
		// weightString is assigned and we have seen an asterisk, now
		// expect a text label.
		case stateLabel:
			t = nextToken()
			switch t.Kind {
			case kindText:
				label = t.Text
				state = stateEndEntry
			default:
				state = stateUnexpected
			}
		// weightString and label are assigned, now emit the entry and
		// expect a comma or EOF.
		case stateEndEntry:
			w, err := strconv.ParseUint(weightString, 10, 32)
			if err != nil {
				return nil, nil, err
			}
			weights = append(weights, uint32(w))
			labels = append(labels, label)
			t = nextToken()
			switch t.Kind {
			case kindEOF:
				state = stateDone
			case kindComma:
				state = stateBeginEntry
			default:
				state = stateUnexpected
			}
		case stateUnexpected:
			if t.Kind == kindError {
				return nil, nil, fmt.Errorf("%s", t.Text)
			} else {
				var ttext string
				switch t.Kind {
				case kindEOF:
					ttext = "end of string"
				case kindComma:
					ttext = "\",\""
				case kindAsterisk:
					ttext = "\"*\""
				case kindText:
					ttext = fmt.Sprintf("%+q", t.Text)
				}
				return nil, nil, fmt.Errorf("unexpected %s", ttext)
			}
		default:
			panic(state)
		}
	}

	return weights, labels, nil
}

// cryptoSource is a math/rand Source that reads from the crypto/rand Reader.
// The Seed method does not affect the sequence of numbers returned from the
// Int63 method.
type cryptoSource struct{}

func (s cryptoSource) Seed(_ int64) {}

func (s cryptoSource) Int63() int64 {
	var n int64
	err := binary.Read(cryptorand.Reader, binary.BigEndian, &n)
	if err != nil {
		panic(err)
	}
	n &= (1 << 63) - 1
	return n
}

// sampleWeighted returns the index of a randomly selected element of the
// weights slice, weighted by the values stored in the slice. Panics if
// the sum of the weights is zero or does not fit in an int64.
func sampleWeighted(weights []uint32) int {
	var sum int64 = 0
	for _, w := range weights {
		sum += int64(w)
		if sum < int64(w) {
			panic("weights overflow")
		}
	}
	if sum == 0 {
		panic("total weight is zero")
	}
	r := uint64(mathrand.New(&cryptoSource{}).Int63n(sum))
	for i, w := range weights {
		if r < uint64(w) {
			return i
		}
		r -= uint64(w)
	}
	panic("impossible")
}
