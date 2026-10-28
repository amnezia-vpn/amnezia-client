/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "dnsutilsmacos.h"

#include <systemconfiguration/scdynamicstore.h>
#include <systemconfiguration/scpreferences.h>

#include <QScopeGuard>
#include <QMutex>
#include <QMutexLocker>
#include <atomic>

#include "leakdetector.h"
#include "logger.h"

namespace {
Logger logger("DnsUtilsMacos");
}

// SystemConfiguration may retain callbacks after asynchronous cancellation.
// Keep their context alive independently of the QObject and serialize access to
// the receiver pointer with its destruction. All DNS work stays on the Qt thread.
struct DnsUtilsMacos::NotificationContext {
  std::atomic<unsigned> references{1};
  QMutex mutex;
  DnsUtilsMacos* receiver = nullptr;
  quint64 generation = 0;

  static const void* retain(const void* info) {
    auto* context = const_cast<NotificationContext*>(
        static_cast<const NotificationContext*>(info));
    ++context->references;
    return info;
  }
  static void release(const void* info) {
    auto* context = const_cast<NotificationContext*>(
        static_cast<const NotificationContext*>(info));
    if (--context->references == 0) delete context;
  }
};

DnsUtilsMacos::DnsUtilsMacos(QObject* parent) : DnsUtils(parent) {
  MZ_COUNT_CTOR(DnsUtilsMacos);
  m_notificationContext = new NotificationContext;
  m_notificationContext->receiver = this;
  SCDynamicStoreContext context = {0, m_notificationContext,
      NotificationContext::retain, NotificationContext::release, nullptr};
  m_scStore = SCDynamicStoreCreate(kCFAllocatorSystemDefault,
                                   CFSTR("amneziavpn"), dnsChanged, &context);
  if (m_scStore == nullptr) {
    logger.error() << "Failed to create system configuration store ref";
  }
  m_notificationQueue = dispatch_queue_create("org.amnezia.dns", DISPATCH_QUEUE_SERIAL);
  logger.debug() << "DnsUtilsMacos created.";
}

DnsUtilsMacos::~DnsUtilsMacos() {
  MZ_COUNT_DTOR(DnsUtilsMacos);
  {
    QMutexLocker lock(&m_notificationContext->mutex);
    m_notificationContext->receiver = nullptr;
  }
  restoreResolvers();
  if (m_scStore) CFRelease(m_scStore);
  if (m_notificationQueue) dispatch_release(m_notificationQueue);
  NotificationContext::release(m_notificationContext);
  logger.debug() << "DnsUtilsMacos destroyed.";
}

void DnsUtilsMacos::dnsChanged(SCDynamicStoreRef, CFArrayRef, void* info) {
  auto* context = static_cast<NotificationContext*>(info);
  QMutexLocker lock(&context->mutex);
  auto* receiver = context->receiver;
  if (!receiver) return;
  const auto generation = context->generation;
  QMetaObject::invokeMethod(receiver, [receiver, generation] {
    if (receiver->m_monitoring && receiver->m_generation == generation &&
        !receiver->applyResolvers()) {
      logger.error() << "Failed to reapply VPN DNS after network change";
    }
  }, Qt::QueuedConnection);
}

bool DnsUtilsMacos::startMonitoring() {
  if (m_monitoring) return true;
  if (!m_scStore || !m_notificationQueue) return false;
  const void* patterns[] = {
      CFSTR("Setup:/Network/Service/[0-9A-F-]+"),
      CFSTR("Setup:/Network/Service/[0-9A-F-]+/DNS")};
  CFArrayRef watched = CFArrayCreate(nullptr, patterns, 2, &kCFTypeArrayCallBacks);
  auto guard = qScopeGuard([&] { CFRelease(watched); });
  if (!SCDynamicStoreSetNotificationKeys(m_scStore, nullptr, watched) ||
      !SCDynamicStoreSetDispatchQueue(m_scStore, m_notificationQueue)) {
    logger.error() << "Failed to monitor DNS configuration changes";
    return false;
  }
  m_monitoring = true;
  return true;
}

void DnsUtilsMacos::stopMonitoring() {
  m_monitoring = false;
  {
    QMutexLocker lock(&m_notificationContext->mutex);
    m_notificationContext->generation = ++m_generation;
  }
  if (m_scStore) SCDynamicStoreSetDispatchQueue(m_scStore, nullptr);
  if (m_dnsConfig) CFRelease(m_dnsConfig);
  m_dnsConfig = nullptr;
}

static QString cfParseString(CFTypeRef ref) {
  if (CFGetTypeID(ref) != CFStringGetTypeID()) {
    return QString();
  }

  CFStringRef stringref = (CFStringRef)ref;
  CFRange range;
  range.location = 0;
  range.length = CFStringGetLength(stringref);
  if (range.length <= 0) {
    return QString();
  }

  UniChar* buf = (UniChar*)malloc(range.length * sizeof(UniChar));
  if (!buf) {
    return QString();
  }
  auto guard = qScopeGuard([&] { free(buf); });

  CFStringGetCharacters(stringref, range, buf);
  return QString::fromUtf16(buf, range.length);
}

static QStringList cfParseStringList(CFTypeRef ref) {
  if (CFGetTypeID(ref) != CFArrayGetTypeID()) {
    return QStringList();
  }

  CFArrayRef array = (CFArrayRef)ref;
  QStringList result;
  for (CFIndex i = 0; i < CFArrayGetCount(array); i++) {
    CFTypeRef value = CFArrayGetValueAtIndex(array, i);
    result.append(cfParseString(value));
  }

  return result;
}

static void cfDictSetString(CFMutableDictionaryRef dict, CFStringRef name,
                            const QString& value) {
  if (value.isNull()) {
    return;
  }
  CFStringRef cfValue = CFStringCreateWithCString(
      kCFAllocatorSystemDefault, qUtf8Printable(value), kCFStringEncodingUTF8);
  CFDictionarySetValue(dict, name, cfValue);
  CFRelease(cfValue);
}

static void cfDictSetStringList(CFMutableDictionaryRef dict, CFStringRef name,
                                const QStringList& valueList) {
  if (valueList.isEmpty()) {
    return;
  }

  CFMutableArrayRef array;
  array = CFArrayCreateMutable(kCFAllocatorSystemDefault, 0,
                               &kCFTypeArrayCallBacks);
  if (array == nullptr) {
    return;
  }

  for (const QString& rstring : valueList) {
    CFStringRef cfAddr = CFStringCreateWithCString(kCFAllocatorSystemDefault,
                                                   qUtf8Printable(rstring),
                                                   kCFStringEncodingUTF8);
    CFArrayAppendValue(array, cfAddr);
    CFRelease(cfAddr);
  }
  CFDictionarySetValue(dict, name, array);
  CFRelease(array);
}

bool DnsUtilsMacos::updateResolvers(const QString& ifname,
                                    const QList<QHostAddress>& resolvers) {
  Q_UNUSED(ifname);

  if (!startMonitoring()) return false;

  // Prepare the DNS configuration.
  CFMutableDictionaryRef dnsConfig = CFDictionaryCreateMutable(
      kCFAllocatorSystemDefault, 0, &kCFCopyStringDictionaryKeyCallBacks,
      &kCFTypeDictionaryValueCallBacks);
  auto configGuard = qScopeGuard([&] { CFRelease(dnsConfig); });
  QStringList list;
  for (const QHostAddress& addr : resolvers) {
    list.append(addr.toString());
  }
  cfDictSetStringList(dnsConfig, kSCPropNetDNSServerAddresses, list);
  cfDictSetString(dnsConfig, kSCPropNetDNSDomainName, "lan");

  if (m_dnsConfig) CFRelease(m_dnsConfig);
  m_dnsConfig = static_cast<CFDictionaryRef>(CFRetain(dnsConfig));
  if (!applyResolvers()) {
    restoreResolvers();
    return false;
  }
  return true;
}

bool DnsUtilsMacos::applyResolvers() {
  if (!m_scStore || !m_dnsConfig) return false;
  CFArrayRef netServices = SCDynamicStoreCopyKeyList(
      m_scStore, CFSTR("Setup:/Network/Service/[0-9A-F-]+"));
  if (!netServices) return false;
  auto serviceGuard = qScopeGuard([&] { CFRelease(netServices); });
  bool success = true;
  // Backup once per connection, including services added while connected.
  for (CFIndex i = 0; i < CFArrayGetCount(netServices); i++) {
    QString service = cfParseString(CFArrayGetValueAtIndex(netServices, i));
    QString uuid = service.section('/', 3, 3);
    if (uuid.isEmpty()) {
      continue;
    }
    if (!m_prevServices.contains(uuid) && !backupService(uuid)) {
      success = false;
      continue;
    }

    CFStringRef dnsPath = CFStringCreateWithFormat(
        kCFAllocatorSystemDefault, nullptr,
        CFSTR("Setup:/Network/Service/%s/DNS"), qPrintable(uuid));
    if (!dnsPath) {
      success = false;
      continue;
    }
    CFPropertyListRef current = SCDynamicStoreCopyValue(m_scStore, dnsPath);
    if (!current && SCError() != kSCStatusNoKey) {
      logger.error() << "Failed to read DNS config for" << uuid;
      success = false;
      CFRelease(dnsPath);
      continue;
    }
    // Our writes also trigger notifications. Avoid a self-sustaining loop.
    if (!current || !CFEqual(current, m_dnsConfig)) {
      logger.debug() << "Setting DNS config for" << uuid;
      if (!SCDynamicStoreSetValue(m_scStore, dnsPath, m_dnsConfig)) {
        logger.error() << "Failed to set DNS config for" << uuid;
        success = false;
      }
    }
    if (current) CFRelease(current);
    CFRelease(dnsPath);
  }
  return success;
}

bool DnsUtilsMacos::restoreResolvers() {
  stopMonitoring();
  if (!m_scStore) return m_prevServices.isEmpty();
  bool success = true;
  for (const QString& uuid : m_prevServices.keys()) {
    CFStringRef path = CFStringCreateWithFormat(
        kCFAllocatorSystemDefault, nullptr,
        CFSTR("Setup:/Network/Service/%s/DNS"), qPrintable(uuid));

    // A deleted network service must not be resurrected by disconnecting.
    CFStringRef servicePath = CFStringCreateWithFormat(nullptr, nullptr,
        CFSTR("Setup:/Network/Service/%s"), qPrintable(uuid));
    CFPropertyListRef service = SCDynamicStoreCopyValue(m_scStore, servicePath);
    const int serviceError = service ? kSCStatusOK : SCError();
    CFRelease(servicePath);
    if (!service) {
      if (serviceError == kSCStatusNoKey) {
        m_prevServices.remove(uuid);
      } else {
        logger.error() << "Failed to read network service during DNS restore" << uuid;
        success = false;
      }
      CFRelease(path);
      continue;
    }
    CFRelease(service);
    bool restored;
    logger.debug() << "Restoring DNS config for" << uuid;
    const DnsBackup& backup = m_prevServices[uuid];
    if (backup.isValid()) {
      CFMutableDictionaryRef config;
      config = CFDictionaryCreateMutable(kCFAllocatorSystemDefault, 0,
                                         &kCFCopyStringDictionaryKeyCallBacks,
                                         &kCFTypeDictionaryValueCallBacks);

      cfDictSetString(config, kSCPropNetDNSDomainName, backup.m_domain);
      cfDictSetStringList(config, kSCPropNetDNSSearchDomains, backup.m_search);
      cfDictSetStringList(config, kSCPropNetDNSServerAddresses,
                          backup.m_servers);
      cfDictSetStringList(config, kSCPropNetDNSSortList, backup.m_sortlist);
      restored = SCDynamicStoreSetValue(m_scStore, path, config);
      CFRelease(config);
    } else {
      CFPropertyListRef current = SCDynamicStoreCopyValue(m_scStore, path);
      restored = current ? SCDynamicStoreRemoveValue(m_scStore, path)
                         : SCError() == kSCStatusNoKey;
      if (current) CFRelease(current);
    }
    if (restored) {
      m_prevServices.remove(uuid);
    } else {
      logger.error() << "Failed to restore DNS config for" << uuid;
      success = false;
    }
    CFRelease(path);
  }
  return success;
}

bool DnsUtilsMacos::backupService(const QString& uuid) {
  DnsBackup backup;
  CFStringRef path = CFStringCreateWithFormat(
      kCFAllocatorSystemDefault, nullptr,
      CFSTR("Setup:/Network/Service/%s/DNS"), qPrintable(uuid));
  CFDictionaryRef config =
      (CFDictionaryRef)SCDynamicStoreCopyValue(m_scStore, path);
  auto serviceGuard = qScopeGuard([&] {
    if (config) {
      CFRelease(config);
    }
    CFRelease(path);
  });

  if (!config && SCError() != kSCStatusNoKey) {
    logger.error() << "Failed to back up DNS config for" << uuid;
    return false;
  }
  // Parse the DNS protocol entry and save it for later.
  if (config) {
    CFTypeRef value;
    value = CFDictionaryGetValue(config, kSCPropNetDNSDomainName);
    if (value) {
      backup.m_domain = cfParseString(value);
    }
    value = CFDictionaryGetValue(config, kSCPropNetDNSServerAddresses);
    if (value) {
      backup.m_servers = cfParseStringList(value);
    }
    value = CFDictionaryGetValue(config, kSCPropNetDNSSearchDomains);
    if (value) {
      backup.m_search = cfParseStringList(value);
    }
    value = CFDictionaryGetValue(config, kSCPropNetDNSSortList);
    if (value) {
      backup.m_sortlist = cfParseStringList(value);
    }
  }

  m_prevServices[uuid] = backup;
  return true;
}
