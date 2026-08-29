package main

/*
#include "jni_helper.h"
*/
import "C"

import (
	"log"
	"strings"
	"sync"
	"unsafe"

	"org.amnezia.vpn/dnstt/dnsttclient"
)

var (
	activeClient *dnsttclient.Client
	clientMu     sync.Mutex
)

// jniLogWriter forwards the Go standard logger to android.util.Log. Without it
// everything libdnstt logs would be invisible on device.
type jniLogWriter struct{}

func (jniLogWriter) Write(p []byte) (int, error) {
	msg := strings.TrimRight(string(p), "\n")
	if msg != "" {
		cMsg := C.CString(msg)
		C._nativeLog(cMsg)
		C.free(unsafe.Pointer(cMsg))
	}
	return len(p), nil
}

func init() {
	log.SetFlags(0)
	log.SetOutput(jniLogWriter{})
}

// protectSocket asks the Android layer to exclude fd from the VPN routes.
func protectSocket(fd int) bool {
	return C._protectSocket(C.int(fd)) != 0
}

// notifyState forwards a tunnel state transition to the Kotlin layer.
func notifyState(state string) {
	cState := C.CString(state)
	defer C.free(unsafe.Pointer(cState))
	C._notifyState(cState)
}

//export Java_org_amnezia_vpn_protocol_dnstt_DnsttNative_startTunnel
func Java_org_amnezia_vpn_protocol_dnstt_DnsttNative_startTunnel(
	env *C.JNIEnv,
	thiz C.jobject,
	tunFd C.jint,
	tunMtu C.jint,
	jDomain C.jstring,
	jResolvers C.jstring,
	jBootstrapIP C.jstring,
	jPubKey C.jstring,
) C.jstring {
	clientMu.Lock()
	defer clientMu.Unlock()

	if activeClient != nil {
		activeClient.Stop()
		activeClient = nil
	}

	cfg := dnsttclient.Config{
		TunFd:       int(tunFd),
		TunMtu:      int(tunMtu),
		Domain:      jstringToString(env, jDomain),
		Resolvers:   jstringToString(env, jResolvers),
		BootstrapIP: jstringToString(env, jBootstrapIP),
		PubKeyHex:   jstringToString(env, jPubKey),
	}

	c, err := dnsttclient.NewClient(cfg, protectSocket, notifyState)
	if err != nil {
		return stringToJstring(env, err.Error())
	}
	if err := c.Start(); err != nil {
		return stringToJstring(env, err.Error())
	}

	activeClient = c
	// A null return means success; any string is the failure reason.
	return 0
}

//export Java_org_amnezia_vpn_protocol_dnstt_DnsttNative_stopTunnel
func Java_org_amnezia_vpn_protocol_dnstt_DnsttNative_stopTunnel(env *C.JNIEnv, thiz C.jobject) C.jstring {
	clientMu.Lock()
	defer clientMu.Unlock()

	if activeClient == nil {
		return 0
	}

	err := activeClient.Stop()
	activeClient = nil
	if err != nil {
		return stringToJstring(env, err.Error())
	}
	return 0
}

//export Java_org_amnezia_vpn_protocol_dnstt_DnsttNative_calculateMtu
func Java_org_amnezia_vpn_protocol_dnstt_DnsttNative_calculateMtu(env *C.JNIEnv, thiz C.jobject, jDomain C.jstring) C.jint {
	return C.jint(dnsttclient.CalculateMtu(jstringToString(env, jDomain)))
}

func jstringToString(env *C.JNIEnv, jstr C.jstring) string {
	if jstr == 0 {
		return ""
	}
	var isCopy C.jboolean
	cStr := C._getStringUTFChars(env, jstr, &isCopy)
	if cStr == nil {
		return ""
	}
	defer C._releaseStringUTFChars(env, jstr, cStr)
	return C.GoString(cStr)
}

func stringToJstring(env *C.JNIEnv, s string) C.jstring {
	cStr := C.CString(s)
	defer C.free(unsafe.Pointer(cStr))
	return C._newStringUTF(env, cStr)
}

func main() {}
