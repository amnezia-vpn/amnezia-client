/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "dnsutilslinux.h"

#include <net/if.h>

#include <QDBusVariant>
#include <QNetworkInterface>
#include <QTimer>
#include <QtDBus/QtDBus>

#include "core/utils/networkUtilities.h"
#include "leakdetector.h"
#include "logger.h"

constexpr const char* DBUS_RESOLVE_SERVICE = "org.freedesktop.resolve1";
constexpr const char* DBUS_RESOLVE_PATH = "/org/freedesktop/resolve1";
constexpr const char* DBUS_RESOLVE_MANAGER = "org.freedesktop.resolve1.Manager";
constexpr const char* DBUS_PROPERTY_INTERFACE =
    "org.freedesktop.DBus.Properties";

namespace {
Logger logger("DnsUtilsLinux");
}

DnsUtilsLinux::DnsUtilsLinux(QObject* parent) : DnsUtils(parent) {
  MZ_COUNT_CTOR(DnsUtilsLinux);
  logger.debug() << "DnsUtilsLinux created.";

  QDBusConnection conn = QDBusConnection::systemBus();
  auto* watcher = new QDBusServiceWatcher(
      DBUS_RESOLVE_SERVICE, conn,
      QDBusServiceWatcher::WatchForRegistration |
      QDBusServiceWatcher::WatchForUnregistration, this);

  connect(watcher, &QDBusServiceWatcher::serviceRegistered,
          this, &DnsUtilsLinux::onResolverRegistered);
  connect(watcher, &QDBusServiceWatcher::serviceUnregistered,
          this, &DnsUtilsLinux::onResolverUnregistered);

  if (conn.interface()->isServiceRegistered(DBUS_RESOLVE_SERVICE)) {
    onResolverRegistered();
  }
}

void DnsUtilsLinux::onResolverRegistered() {
  m_resolver.reset(new QDBusInterface(DBUS_RESOLVE_SERVICE, DBUS_RESOLVE_PATH,
                                      DBUS_RESOLVE_MANAGER,
                                      QDBusConnection::systemBus()));
  logger.debug() << "systemd-resolved available, DNS resolver initialized";

  if (m_revertAfterRegister > 0) {
    logger.debug() << "Calling RevertLink after restart for ifindex" << m_revertAfterRegister;
    QDBusMessage msg = QDBusMessage::createMethodCall(
        DBUS_RESOLVE_SERVICE, DBUS_RESOLVE_PATH, DBUS_RESOLVE_MANAGER, "RevertLink");
    msg.setArguments({QVariant::fromValue(m_revertAfterRegister)});
    QDBusPendingReply<> reply = QDBusConnection::systemBus().asyncCall(msg, 5000);
    int savedIdx = m_revertAfterRegister;
    m_revertAfterRegister = 0;
    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(reply, this);
    QObject::connect(watcher, &QDBusPendingCallWatcher::finished, this,
                     [this, savedIdx](QDBusPendingCallWatcher* w) {
                       QDBusPendingReply<> r = *w;
                       if (r.isError()) {
                         logger.debug() << "RevertLink after restart failed for ifindex" << savedIdx
                                        << ":" << r.error().message();
                       } else {
                         logger.debug() << "RevertLink after restart succeeded for ifindex" << savedIdx;
                       }
                       w->deleteLater();
                     });
  }

  if (!m_pendingIfname.isEmpty()) {
    logger.debug() << "Re-applying DNS configuration for" << m_pendingIfname;
    updateResolvers(m_pendingIfname, m_pendingResolvers);
  }
}

void DnsUtilsLinux::onResolverUnregistered() {
  logger.debug() << "systemd-resolved disappeared, dropping DNS resolver";
  m_resolver.reset();
}

DnsUtilsLinux::~DnsUtilsLinux() {
  MZ_COUNT_DTOR(DnsUtilsLinux);

  if (m_revertOnDestroy && m_resolver) {
    if (m_gatewayIfindex > 0)
      setLinkDefaultRoute(m_gatewayIfindex, true);

    for (auto iterator = m_linkDomains.constBegin();
         iterator != m_linkDomains.constEnd(); ++iterator) {
      QList<QVariant> argumentList;
      argumentList << QVariant::fromValue(iterator.key());
      argumentList << QVariant::fromValue(iterator.value());
      m_resolver->asyncCallWithArgumentList(QStringLiteral("SetLinkDomains"),
                                            argumentList);
    }
    if (m_ifindex > 0) {
      m_resolver->asyncCall(QStringLiteral("RevertLink"), m_ifindex);
    }
  }

  logger.debug() << "DnsUtilsLinux destroyed.";
}

bool DnsUtilsLinux::updateResolvers(const QString& ifname,
                                    const QList<QHostAddress>& resolvers) {
  m_revertAfterRegister = 0;

  if (m_gatewayIfindex > 0) {
    setLinkDefaultRoute(m_gatewayIfindex, true);
    m_gatewayIfindex = 0;
  }

  m_ifindex = if_nametoindex(qPrintable(ifname));
  if (m_ifindex <= 0) {
    logger.error() << "Unable to resolve ifindex for" << ifname;
    return false;
  }

  // Reset retry counter only when called externally (not from scheduleRetry)
  if (ifname != m_pendingIfname || resolvers != m_pendingResolvers)
    m_domainRetries = 0;

  m_pendingIfname = ifname;
  m_pendingResolvers = resolvers;

  if (!m_resolver) {
    logger.debug() << "systemd-resolved not ready, queuing DNS configuration";
    return true;
  }

  const int gwIdx = NetworkUtilities::getGatewayAndIface().second.index();
  if (gwIdx > 0 && gwIdx != m_ifindex && gwIdx != m_gatewayIfindex) {
    m_gatewayIfindex = gwIdx;
    setLinkDefaultRoute(gwIdx, false);
  }

  setLinkDNS(m_ifindex, resolvers);
  setLinkDefaultRoute(m_ifindex, true);
  updateLinkDomains();
  return true;
}

bool DnsUtilsLinux::restoreResolvers() {
  m_revertOnDestroy = true;
  m_pendingIfname.clear();
  m_pendingResolvers.clear();

  if (m_gatewayIfindex > 0) {
    setLinkDefaultRoute(m_gatewayIfindex, true);
    m_gatewayIfindex = 0;
  }

  for (auto iterator = m_linkDomains.constBegin();
       iterator != m_linkDomains.constEnd(); ++iterator) {
    setLinkDomains(iterator.key(), iterator.value());
  }
  m_linkDomains.clear();

  /* Revert the VPN interface DNS. Save the ifindex so that
   * onResolverRegistered() can call RevertLink again after flushDns()
   * restarts systemd-resolved (handles the case where systemd-resolved
   * is unresponsive at disconnect time). */
  if (m_ifindex > 0) {
    m_revertAfterRegister = m_ifindex;
    m_ifindex = 0;
  }

  return true;
}

void DnsUtilsLinux::scheduleRetry() {
  if (m_pendingIfname.isEmpty() || m_retryPending || m_domainRetries >= 5)
    return;
  m_retryPending = true;
  ++m_domainRetries;
  logger.debug() << "Retrying full DNS setup (" << m_domainRetries << "/5)";
  QTimer::singleShot(1000, this, [this]() {
    m_retryPending = false;
    if (!m_pendingIfname.isEmpty())
      updateResolvers(m_pendingIfname, m_pendingResolvers);
  });
}

void DnsUtilsLinux::dnsCallCompleted(QDBusPendingCallWatcher* call) {
  QDBusPendingReply<> reply = *call;
  if (reply.isError()) {
    logger.debug() << "DBus call failed (may be transient after systemd-resolved restart)";
    scheduleRetry();
  }
  delete call;
}

void DnsUtilsLinux::setLinkDNS(int ifindex,
                               const QList<QHostAddress>& resolvers) {
  if (!m_resolver) return;
  QList<DnsResolver> resolverList;
  char ifnamebuf[IF_NAMESIZE];
  const char* ifname = if_indextoname(ifindex, ifnamebuf);
  for (const auto& ip : resolvers) {
    resolverList.append(ip);
    if (ifname) {
      logger.debug() << "Adding DNS resolver" << ip.toString() << "via"
                     << ifname;
    }
  }

  QList<QVariant> argumentList;
  argumentList << QVariant::fromValue(ifindex);
  argumentList << QVariant::fromValue(resolverList);
  QDBusMessage msg = QDBusMessage::createMethodCall(
      DBUS_RESOLVE_SERVICE, DBUS_RESOLVE_PATH, DBUS_RESOLVE_MANAGER, "SetLinkDNS");
  msg.setArguments(argumentList);
  QDBusPendingReply<> reply = QDBusConnection::systemBus().asyncCall(msg, 5000);

  QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(reply, this);
  QObject::connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), this,
                   SLOT(dnsCallCompleted(QDBusPendingCallWatcher*)));
}

void DnsUtilsLinux::setLinkDomains(int ifindex,
                                   const QList<DnsLinkDomain>& domains) {
  if (!m_resolver) return;
  char ifnamebuf[IF_NAMESIZE];
  const char* ifname = if_indextoname(ifindex, ifnamebuf);
  if (ifname) {
    for (const auto& d : domains) {
      // The DNS search domains often winds up revealing user's ISP which
      // can correlate back to their location.
      logger.debug() << "Setting DNS domain:" << logger.sensitive(d.domain)
                     << "via" << ifname << (d.search ? "search" : "");
    }
  }

  QList<QVariant> argumentList;
  argumentList << QVariant::fromValue(ifindex);
  argumentList << QVariant::fromValue(domains);
  QDBusMessage msg = QDBusMessage::createMethodCall(
      DBUS_RESOLVE_SERVICE, DBUS_RESOLVE_PATH, DBUS_RESOLVE_MANAGER, "SetLinkDomains");
  msg.setArguments(argumentList);
  QDBusPendingReply<> reply = QDBusConnection::systemBus().asyncCall(msg, 5000);

  QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(reply, this);
  QObject::connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), this,
                   SLOT(dnsCallCompleted(QDBusPendingCallWatcher*)));
}

void DnsUtilsLinux::setLinkDefaultRoute(int ifindex, bool enable) {
  if (!m_resolver) return;
  QList<QVariant> argumentList;
  argumentList << QVariant::fromValue(ifindex);
  argumentList << QVariant::fromValue(enable);
  QDBusMessage msg = QDBusMessage::createMethodCall(
      DBUS_RESOLVE_SERVICE, DBUS_RESOLVE_PATH, DBUS_RESOLVE_MANAGER, "SetLinkDefaultRoute");
  msg.setArguments(argumentList);
  QDBusPendingReply<> reply = QDBusConnection::systemBus().asyncCall(msg, 5000);

  QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(reply, this);
  QObject::connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), this,
                   SLOT(dnsCallCompleted(QDBusPendingCallWatcher*)));
}

void DnsUtilsLinux::updateLinkDomains() {
  if (!m_resolver) return;
  /* Get the list of search domains, and remove any others that might conspire
   * to satisfy DNS resolution. Unfortunately, this is a pain because Qt doesn't
   * seem to be able to demarshall complex property types.
   */
  QDBusMessage message = QDBusMessage::createMethodCall(
      DBUS_RESOLVE_SERVICE, DBUS_RESOLVE_PATH, DBUS_PROPERTY_INTERFACE, "Get");
  message << QString(DBUS_RESOLVE_MANAGER);
  message << QString("Domains");
  QDBusPendingReply<QVariant> reply =
      m_resolver->connection().asyncCall(message, 5000);

  QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(reply, this);
  QObject::connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), this,
                   SLOT(dnsDomainsReceived(QDBusPendingCallWatcher*)));
}

void DnsUtilsLinux::dnsDomainsReceived(QDBusPendingCallWatcher* call) {
  QDBusPendingReply<QVariant> reply = *call;
  call->deleteLater();
  if (reply.isError()) {
    logger.debug() << "DBus Domains call failed (may be transient after systemd-resolved restart)";
    scheduleRetry();
    return;
  }
  m_domainRetries = 0;

  /* Update the state of the DNS domains */
  m_linkDomains.clear();
  QDBusArgument args = qvariant_cast<QDBusArgument>(reply.value());
  QList<DnsDomain> list = qdbus_cast<QList<DnsDomain>>(args);
  for (const auto& d : list) {
    if (d.ifindex == 0) {
      continue;
    }
    m_linkDomains[d.ifindex].append(DnsLinkDomain(d.domain, d.search));
  }

  /* Drop any competing root search domains. */
  DnsLinkDomain root = DnsLinkDomain(".", true);
  for (auto iterator = m_linkDomains.constBegin();
       iterator != m_linkDomains.constEnd(); ++iterator) {
    if (!iterator.value().contains(root)) {
      continue;
    }
    QList<DnsLinkDomain> newlist = iterator.value();
    newlist.removeAll(root);
    setLinkDomains(iterator.key(), newlist);
  }

  /* Add a root search domain for the new interface. */
  if (m_ifindex > 0) {
    setLinkDomains(m_ifindex, {root});

    /* Disable DefaultRoute on the physical gateway so systemd-resolved
     * routes all DNS through the VPN interface. */
    const int gwIdx = NetworkUtilities::getGatewayAndIface().second.index();
    if (gwIdx > 0 && gwIdx != m_ifindex && gwIdx != m_gatewayIfindex) {
      m_gatewayIfindex = gwIdx;
      setLinkDefaultRoute(gwIdx, false);
    }
  }
}

static DnsMetatypeRegistrationProxy s_dnsMetatypeProxy;
