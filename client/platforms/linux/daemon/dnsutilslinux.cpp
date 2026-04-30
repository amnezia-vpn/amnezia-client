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

  if (m_resolver) {
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
  if (m_gatewayIfindex > 0) {
    setLinkDefaultRoute(m_gatewayIfindex, true);
    m_gatewayIfindex = 0;
  }

  m_ifindex = if_nametoindex(qPrintable(ifname));
  if (m_ifindex <= 0) {
    logger.error() << "Unable to resolve ifindex for" << ifname;
    return false;
  }

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

  /* Revert the VPN interface's DNS configuration */
  if (m_ifindex > 0 && m_resolver) {
    QList<QVariant> argumentList = {QVariant::fromValue(m_ifindex)};
    QDBusPendingReply<> reply = m_resolver->asyncCallWithArgumentList(
        QStringLiteral("RevertLink"), argumentList);

    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(reply, this);
    QObject::connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), this,
                     SLOT(dnsCallCompleted(QDBusPendingCallWatcher*)));

    m_ifindex = 0;
  }

  return true;
}

void DnsUtilsLinux::dnsCallCompleted(QDBusPendingCallWatcher* call) {
  QDBusPendingReply<> reply = *call;
  if (reply.isError()) {
    logger.debug() << "DBus call failed (may be transient after systemd-resolved restart)";
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
  QDBusPendingReply<> reply = m_resolver->asyncCallWithArgumentList(
      QStringLiteral("SetLinkDNS"), argumentList);

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
  QDBusPendingReply<> reply = m_resolver->asyncCallWithArgumentList(
      QStringLiteral("SetLinkDomains"), argumentList);

  QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(reply, this);
  QObject::connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), this,
                   SLOT(dnsCallCompleted(QDBusPendingCallWatcher*)));
}

void DnsUtilsLinux::setLinkDefaultRoute(int ifindex, bool enable) {
  if (!m_resolver) return;
  QList<QVariant> argumentList;
  argumentList << QVariant::fromValue(ifindex);
  argumentList << QVariant::fromValue(enable);
  QDBusPendingReply<> reply = m_resolver->asyncCallWithArgumentList(
      QStringLiteral("SetLinkDefaultRoute"), argumentList);

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
      m_resolver->connection().asyncCall(message);

  QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(reply, this);
  QObject::connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), this,
                   SLOT(dnsDomainsReceived(QDBusPendingCallWatcher*)));
}

void DnsUtilsLinux::dnsDomainsReceived(QDBusPendingCallWatcher* call) {
  QDBusPendingReply<QVariant> reply = *call;
  call->deleteLater();
  if (reply.isError()) {
    // systemd-resolved may still be starting up after a restart — retry a few times
    if (m_ifindex > 0 && m_domainRetries++ < 5) {
      logger.debug() << "systemd-resolved not ready yet, retrying DNS setup ("
                     << m_domainRetries << "/5)";
      QTimer::singleShot(500, this, &DnsUtilsLinux::updateLinkDomains);
    } else {
      logger.warning() << "Failed to configure DNS after 5 retries";
      m_domainRetries = 0;
    }
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
