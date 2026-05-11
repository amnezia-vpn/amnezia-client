// SPDX-License-Identifier: GPL-3.0-or-later
//
// JNI bridge for the Android VpnService → native MasterDnsVPN engine path.
//
// Compiled into the main Qt-for-Android shared library (the same .so the
// Java loader pulls in for the Activity), so no extra `loadLibrary` call
// is needed on the Kotlin side beyond what Qt already does.
//
// The Android architecture is meaningfully different from desktop:
//   * No privileged service daemon — the VpnService runs in the same
//     process as the activity / Qt SO.
//   * Engine runs in-process inside that same SO.
//   * tun2socks is provided by libxray.aar (already a dep) and is
//     called from Kotlin, which feeds it the SOCKS5 port we expose
//     here.

#include "engine.h"

#include <jni.h>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <atomic>
#include <memory>
#include <mutex>

namespace {

// One Engine per JVM process — the Android UX is "one tunnel at a time"
// and the engine is heavy enough that we don't want multiple instances.
std::mutex g_engineMutex;
std::unique_ptr<amnezia::masterdnsvpn::Engine> g_engine;

QString jstringToQString(JNIEnv *env, jstring s)
{
    if (!s) {
        return {};
    }
    const char *raw = env->GetStringUTFChars(s, nullptr);
    QString out = QString::fromUtf8(raw);
    env->ReleaseStringUTFChars(s, raw);
    return out;
}

} // namespace

extern "C" {

// ---- Lifecycle -----------------------------------------------------------
//
// Method signature mapping (mangled JNI symbol -> Kotlin):
//
//   org.amnezia.vpn.protocol.masterdnsvpn.MasterDnsVpnNative.nativeStart
//     (Ljava/lang/String;)Z
//
JNIEXPORT jboolean JNICALL
Java_org_amnezia_vpn_protocol_masterdnsvpn_MasterDnsVpnNative_nativeStart(JNIEnv *env,
                                                                          jclass /*clazz*/,
                                                                          jstring configJson)
{
    const QString json = jstringToQString(env, configJson);
    QJsonParseError err {};
    const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "masterdnsvpn JNI: bad config JSON:" << err.errorString();
        return JNI_FALSE;
    }

    std::lock_guard<std::mutex> lock(g_engineMutex);
    if (g_engine) {
        g_engine->stop();
        g_engine.reset();
    }
    g_engine = std::make_unique<amnezia::masterdnsvpn::Engine>();
    if (!g_engine->start(doc.object())) {
        qWarning() << "masterdnsvpn JNI: engine start failed:" << g_engine->lastError();
        g_engine.reset();
        return JNI_FALSE;
    }
    return JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_org_amnezia_vpn_protocol_masterdnsvpn_MasterDnsVpnNative_nativeStop(JNIEnv * /*env*/,
                                                                         jclass /*clazz*/)
{
    std::lock_guard<std::mutex> lock(g_engineMutex);
    if (g_engine) {
        g_engine->stop();
        g_engine.reset();
    }
}

JNIEXPORT jint JNICALL
Java_org_amnezia_vpn_protocol_masterdnsvpn_MasterDnsVpnNative_nativeSocksPort(JNIEnv * /*env*/,
                                                                              jclass /*clazz*/)
{
    std::lock_guard<std::mutex> lock(g_engineMutex);
    return g_engine ? static_cast<jint>(g_engine->socksPort()) : 0;
}

// State enum returned to Kotlin. Mirrors Engine::State integer values so
// the Kotlin side can treat them as a plain ordinal. Callers should only
// rely on the relative ordering (Connected = positive, Failed = negative).
JNIEXPORT jint JNICALL
Java_org_amnezia_vpn_protocol_masterdnsvpn_MasterDnsVpnNative_nativeState(JNIEnv * /*env*/,
                                                                          jclass /*clazz*/)
{
    std::lock_guard<std::mutex> lock(g_engineMutex);
    return g_engine ? static_cast<jint>(g_engine->state())
                    : static_cast<jint>(amnezia::masterdnsvpn::Engine::State::Idle);
}

JNIEXPORT jstring JNICALL
Java_org_amnezia_vpn_protocol_masterdnsvpn_MasterDnsVpnNative_nativeLastError(JNIEnv *env,
                                                                              jclass /*clazz*/)
{
    std::lock_guard<std::mutex> lock(g_engineMutex);
    if (!g_engine) {
        return env->NewStringUTF("");
    }
    return env->NewStringUTF(g_engine->lastError().toUtf8().constData());
}

JNIEXPORT jlong JNICALL
Java_org_amnezia_vpn_protocol_masterdnsvpn_MasterDnsVpnNative_nativeBytesReceived(
        JNIEnv * /*env*/, jclass /*clazz*/)
{
    std::lock_guard<std::mutex> lock(g_engineMutex);
    return g_engine ? static_cast<jlong>(g_engine->bytesReceived()) : 0;
}

JNIEXPORT jlong JNICALL
Java_org_amnezia_vpn_protocol_masterdnsvpn_MasterDnsVpnNative_nativeBytesSent(JNIEnv * /*env*/,
                                                                              jclass /*clazz*/)
{
    std::lock_guard<std::mutex> lock(g_engineMutex);
    return g_engine ? static_cast<jlong>(g_engine->bytesSent()) : 0;
}

} // extern "C"
