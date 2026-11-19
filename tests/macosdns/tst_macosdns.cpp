#include <QtTest>
#include <dispatch/dispatch.h>
#include <systemconfiguration/systemconfiguration.h>

#include "platforms/macos/daemon/dnsutilsmacos.h"

namespace {
const QString service = "Setup:/Network/Service/11111111-1111-1111-1111-111111111111";
const QString dnsPath = service + "/DNS";
const QList<QHostAddress> vpnDns = {QHostAddress("172.29.172.254"), QHostAddress("10.13.37.1")};
QMap<QString, CFPropertyListRef> values;
SCDynamicStoreCallBack callback = nullptr;
void* callbackInfo = nullptr;
SCDynamicStoreRef fakeStore = nullptr;
const void* (*retainInfo)(const void*) = nullptr;
void (*releaseInfo)(const void*) = nullptr;
CFArrayRef watchedPatterns = nullptr;
dispatch_queue_t queue = nullptr;
int writes = 0;
bool failWrite = false;
bool failWatch = false;
bool failCreate = false;
bool failQueue = false;
bool wasUnsubscribed = false;
QString failReadPath;
int lastError = kSCStatusOK;

QString string(CFStringRef value) { return QString::fromCFString(value); }
void put(const QString& path, CFPropertyListRef value) {
  if (values.contains(path)) CFRelease(values.take(path));
  if (value) values.insert(path, CFRetain(value));
}
void setDns(const QString& path, const QStringList& servers) {
  auto dict = CFDictionaryCreateMutable(nullptr, 0, &kCFTypeDictionaryKeyCallBacks,
                                        &kCFTypeDictionaryValueCallBacks);
  auto list = CFArrayCreateMutable(nullptr, 0, &kCFTypeArrayCallBacks);
  for (const auto& server : servers) {
    auto address = server.toCFString();
    CFArrayAppendValue(list, address);
    CFRelease(address);
  }
  CFDictionarySetValue(dict, kSCPropNetDNSServerAddresses, list);
  put(path, dict);
  CFRelease(list);
  CFRelease(dict);
}
QStringList servers(const QString& path = dnsPath) {
  QStringList result;
  auto dict = static_cast<CFDictionaryRef>(values.value(path));
  if (!dict) return result;
  auto list = static_cast<CFArrayRef>(CFDictionaryGetValue(dict, kSCPropNetDNSServerAddresses));
  if (!list) return result;
  for (CFIndex i = 0; i < CFArrayGetCount(list); ++i)
    result.append(string(static_cast<CFStringRef>(CFArrayGetValueAtIndex(list, i))));
  return result;
}
void notify(const QString& changedPath = dnsPath) {
  if (!queue || !callback || !watchedPatterns) return;
  bool watched = false;
  for (CFIndex i = 0; i < CFArrayGetCount(watchedPatterns); ++i) {
    auto pattern = static_cast<CFStringRef>(CFArrayGetValueAtIndex(watchedPatterns, i));
    watched |= QRegularExpression("\\A(?:" + string(pattern) + ")\\z")
                   .match(changedPath).hasMatch();
  }
  if (!watched) return;
  // Drain the native callback before returning; its queued Qt work remains pending.
  auto keys = CFArrayCreate(nullptr, nullptr, 0, &kCFTypeArrayCallBacks);
  dispatch_sync(queue, ^{ callback(fakeStore, keys, callbackInfo); });
  CFRelease(keys);
}
void drain() {
  QCoreApplication::sendPostedEvents();
  QCoreApplication::processEvents();
}
}

// Fake the privileged dynamic store. Tests never read or change host DNS.
extern "C" SCDynamicStoreRef SCDynamicStoreCreate(CFAllocatorRef, CFStringRef,
    SCDynamicStoreCallBack cb, SCDynamicStoreContext* context) {
  if (failCreate) return nullptr;
  callback = cb;
  callbackInfo = context ? context->info : nullptr;
  retainInfo = context ? context->retain : nullptr;
  releaseInfo = context ? context->release : nullptr;
  // The CF object owns the callback context just like the real store, including
  // references retained by callbacks that arrive after cancellation.
  CFArrayCallBacks ownership = {0,
    [](CFAllocatorRef, const void* value) -> const void* {
      return retainInfo ? retainInfo(value) : value;
    },
    [](CFAllocatorRef, const void* value) { if (releaseInfo) releaseInfo(value); },
    nullptr, nullptr};
  const void* info = callbackInfo;
  fakeStore = reinterpret_cast<SCDynamicStoreRef>(
      CFArrayCreate(nullptr, &info, 1, &ownership));
  return fakeStore;
}
extern "C" CFArrayRef SCDynamicStoreCopyKeyList(SCDynamicStoreRef, CFStringRef pattern) {
  auto list = CFArrayCreateMutable(nullptr, 0, &kCFTypeArrayCallBacks);
  QRegularExpression re("\\A(?:" + string(pattern) + ")\\z");
  for (auto it = values.cbegin(); it != values.cend(); ++it) {
    if (re.match(it.key()).hasMatch()) {
      auto key = it.key().toCFString();
      CFArrayAppendValue(list, key);
      CFRelease(key);
    }
  }
  return list;
}
extern "C" CFPropertyListRef SCDynamicStoreCopyValue(SCDynamicStoreRef, CFStringRef key) {
  if (string(key) == failReadPath) {
    lastError = kSCStatusFailed;
    return nullptr;
  }
  auto value = values.value(string(key));
  lastError = value ? kSCStatusOK : kSCStatusNoKey;
  return value ? CFRetain(value) : nullptr;
}
extern "C" int SCError() { return lastError; }
extern "C" Boolean SCDynamicStoreSetValue(SCDynamicStoreRef, CFStringRef key, CFPropertyListRef value) {
  if (failWrite) return false;
  put(string(key), value);
  ++writes;
  return true;
}
extern "C" Boolean SCDynamicStoreRemoveValue(SCDynamicStoreRef, CFStringRef key) {
  if (failWrite) return false;
  put(string(key), nullptr);
  return true;
}
extern "C" Boolean SCDynamicStoreSetNotificationKeys(SCDynamicStoreRef, CFArrayRef, CFArrayRef patterns) {
  if (watchedPatterns) CFRelease(watchedPatterns);
  watchedPatterns = patterns ? static_cast<CFArrayRef>(CFRetain(patterns)) : nullptr;
  return !failWatch;
}
extern "C" Boolean SCDynamicStoreSetDispatchQueue(SCDynamicStoreRef, dispatch_queue_t next) {
  if (next && (failQueue || wasUnsubscribed)) return false;
  if (!next && queue) wasUnsubscribed = true;
  if (next) dispatch_retain(next);
  if (queue) dispatch_release(queue);
  queue = next;
  return true;
}

class MacosDnsTests : public QObject {
  Q_OBJECT
 private slots:
  void init() {
    callback = nullptr; callbackInfo = nullptr;
    writes = 0; failWrite = false; failWatch = false; failCreate = false;
    failQueue = false; wasUnsubscribed = false; failReadPath.clear(); lastError = kSCStatusOK;
    put(service, CFSTR("Wi-Fi"));
  }
  void cleanup() {
    drain();
    for (auto value : values) CFRelease(value);
    values.clear();
    if (queue) dispatch_release(queue);
    queue = nullptr;
    if (watchedPatterns) CFRelease(watchedPatterns);
    watchedPatterns = nullptr;
  }
  void initialApplyAndRestoreDhcp() {
    DnsUtilsMacos dns(nullptr);
    QVERIFY(dns.updateResolvers("utun11", vpnDns));
    QCOMPARE(servers(), QStringList({"172.29.172.254", "10.13.37.1"}));
    QVERIFY(dns.restoreResolvers());
    QVERIFY(!values.contains(dnsPath));
  }
  void restoresStaticDns() {
    setDns(dnsPath, {"192.0.2.53"});
    DnsUtilsMacos dns(nullptr);
    QVERIFY(dns.updateResolvers("utun11", vpnDns));
    QVERIFY(dns.restoreResolvers());
    QCOMPARE(servers(), QStringList({"192.0.2.53"}));
  }
  void repairsDeletedOverride() {
    DnsUtilsMacos dns(nullptr);
    QVERIFY(dns.updateResolvers("utun11", vpnDns));
    put(dnsPath, nullptr);
    notify();
    QTRY_COMPARE(servers(), QStringList({"172.29.172.254", "10.13.37.1"}));
  }
  void repairsReplacedOverrideAndPreservesBackup() {
    setDns(dnsPath, {"192.0.2.53"});
    DnsUtilsMacos dns(nullptr);
    QVERIFY(dns.updateResolvers("utun11", vpnDns));
    setDns(dnsPath, {"192.168.1.1"});
    notify();
    QTRY_COMPARE(servers(), QStringList({"172.29.172.254", "10.13.37.1"}));
    QVERIFY(dns.restoreResolvers());
    QCOMPARE(servers(), QStringList({"192.0.2.53"}));
  }
  void repeatedUpdatesPreserveBackup() {
    setDns(dnsPath, {"192.0.2.53"});
    DnsUtilsMacos dns(nullptr);
    QVERIFY(dns.updateResolvers("utun11", vpnDns));
    QVERIFY(dns.updateResolvers("utun11", vpnDns));
    QVERIFY(dns.restoreResolvers());
    QCOMPARE(servers(), QStringList({"192.0.2.53"}));
  }
  void ownNotificationsDoNotRewriteDns() {
    DnsUtilsMacos dns(nullptr);
    QVERIFY(dns.updateResolvers("utun11", vpnDns));
    const int before = writes;
    notify(); drain(); notify(); drain();
    QCOMPARE(writes, before);
  }
  void queuedNotificationCannotUndoDisconnect() {
    DnsUtilsMacos dns(nullptr);
    QVERIFY(dns.updateResolvers("utun11", vpnDns));
    put(dnsPath, nullptr);
    notify();
    QVERIFY(dns.restoreResolvers());
    drain();
    QVERIFY(!values.contains(dnsPath));
  }
  void newServiceGetsVpnDnsAndRestoresItsOwnDns() {
    DnsUtilsMacos dns(nullptr);
    QVERIFY(dns.updateResolvers("utun11", vpnDns));
    const QString added = "Setup:/Network/Service/22222222-2222-2222-2222-222222222222";
    put(added, CFSTR("USB"));
    setDns(added + "/DNS", {"192.0.2.54"});
    notify(added);
    QTRY_COMPARE(servers(added + "/DNS"), QStringList({"172.29.172.254", "10.13.37.1"}));
    QVERIFY(dns.restoreResolvers());
    QCOMPARE(servers(added + "/DNS"), QStringList({"192.0.2.54"}));
  }
  void writeFailureIsReported() {
    DnsUtilsMacos dns(nullptr);
    failWrite = true;
    QVERIFY(!dns.updateResolvers("utun11", vpnDns));
    failWrite = false;
  }
  void storeCreationFailureIsReported() {
    failCreate = true;
    DnsUtilsMacos dns(nullptr);
    QVERIFY(!dns.updateResolvers("utun11", vpnDns));
    QVERIFY(!values.contains(dnsPath));
  }
  void disconnectRestorationFailureCanBeRetried() {
    setDns(dnsPath, {"192.0.2.53"});
    DnsUtilsMacos dns(nullptr);
    QVERIFY(dns.updateResolvers("utun11", vpnDns));
    failWrite = true;
    QVERIFY(!dns.restoreResolvers());
    failWrite = false;
    QVERIFY(dns.restoreResolvers());
    QCOMPARE(servers(), QStringList({"192.0.2.53"}));
  }
  void removedServiceIsNotResurrected() {
    setDns(dnsPath, {"192.0.2.53"});
    DnsUtilsMacos dns(nullptr);
    QVERIFY(dns.updateResolvers("utun11", vpnDns));
    put(service, nullptr); put(dnsPath, nullptr);
    notify(service); drain();
    QVERIFY(!values.contains(dnsPath));
    QVERIFY(dns.restoreResolvers());
    QVERIFY(!values.contains(dnsPath));
  }
  void staleQueuedRepairCannotModifyNewConnection() {
    DnsUtilsMacos dns(nullptr);
    QVERIFY(dns.updateResolvers("utun11", vpnDns));
    notify();
    QVERIFY(dns.restoreResolvers());
    QVERIFY(dns.updateResolvers("utun12", {QHostAddress("192.0.2.54")}));
    // Remove the new override without a new notification. An old queued callback
    // must not process this connection, even using its current desired settings.
    put(dnsPath, nullptr);
    drain();
    QVERIFY(!values.contains(dnsPath));
    notify();
    QTRY_COMPARE(servers(), QStringList({"192.0.2.54"}));
  }
  void callbacksAfterDestructionAreSafe() {
    auto* dns = new DnsUtilsMacos(nullptr);
    QVERIFY(dns->updateResolvers("utun11", vpnDns));
    notify();  // Queued Qt callback.
    auto retainedStore = static_cast<SCDynamicStoreRef>(CFRetain(fakeStore));
    auto lateCallback = callback;
    auto lateInfo = callbackInfo;
    delete dns;
    drain();
    auto keys = CFArrayCreate(nullptr, nullptr, 0, &kCFTypeArrayCallBacks);
    lateCallback(retainedStore, keys, lateInfo);  // Late native callback.
    CFRelease(keys);
    CFRelease(retainedStore);
    drain();
    QVERIFY(!values.contains(dnsPath));
  }
  void serviceReadFailureRetainsBackupForRetry() {
    setDns(dnsPath, {"192.0.2.53"});
    DnsUtilsMacos dns(nullptr);
    QVERIFY(dns.updateResolvers("utun11", vpnDns));
    failReadPath = service;
    QVERIFY(!dns.restoreResolvers());
    failReadPath.clear();
    QVERIFY(dns.restoreResolvers());
    QCOMPARE(servers(), QStringList({"192.0.2.53"}));
  }
  void dnsReadFailureRetainsDhcpBackupForRetry() {
    DnsUtilsMacos dns(nullptr);
    QVERIFY(dns.updateResolvers("utun11", vpnDns));
    failReadPath = dnsPath;
    QVERIFY(!dns.restoreResolvers());
    failReadPath.clear();
    QVERIFY(dns.restoreResolvers());
    QVERIFY(!values.contains(dnsPath));
  }
  void backupReadFailureMustNotOverwriteStaticDns() {
    setDns(dnsPath, {"192.0.2.53"});
    DnsUtilsMacos dns(nullptr);
    failReadPath = dnsPath;
    QVERIFY(!dns.updateResolvers("utun11", vpnDns));
    QCOMPARE(servers(), QStringList({"192.0.2.53"}));
    failReadPath.clear();
  }
  void dispatchRegistrationFailureIsReported() {
    DnsUtilsMacos dns(nullptr);
    failQueue = true;
    QVERIFY(!dns.updateResolvers("utun11", vpnDns));
    QVERIFY(!values.contains(dnsPath));
  }
  void notificationFailureIsReported() {
    DnsUtilsMacos dns(nullptr);
    failWatch = true;
    QVERIFY(!dns.updateResolvers("utun11", vpnDns));
    QVERIFY(!values.contains(dnsPath));
  }
};
QTEST_GUILESS_MAIN(MacosDnsTests)
#include "tst_macosdns.moc"
