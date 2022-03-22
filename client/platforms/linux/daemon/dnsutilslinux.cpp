/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "dnsutilslinux.h"
#include "leakdetector.h"
#include "logger.h"

#include <QtDBus/QtDBus>
#include <QDBusVariant>

#include <net/if.h>

constexpr const char* DBUS_RESOLVE_SERVICE = "org.freedesktop.resolve1";
constexpr const char* DBUS_RESOLVE_PATH = "/org/freedesktop/resolve1";
constexpr const char* DBUS_RESOLVE_MANAGER = "org.freedesktop.resolve1.Manager";
constexpr const char* DBUS_PROPERTY_INTERFACE =
    "org.freedesktop.DBus.Properties";

namespace {
Logger logger(LOG_LINUX, "DnsUtilsLinux");
}

DnsUtilsLinux::DnsUtilsLinux(QObject* parent) : DnsUtils(parent) {
  MVPN_COUNT_CTOR(DnsUtilsLinux);
  logger.debug() << "DnsUtilsLinux created.";

  QDBusConnection conn = QDBusConnection::systemBus();
  m_resolver = new QDBusInterface(DBUS_RESOLVE_SERVICE, DBUS_RESOLVE_PATH,
                                  DBUS_RESOLVE_MANAGER, conn, this);
}

DnsUtilsLinux::~DnsUtilsLinux() {
  MVPN_COUNT_DTOR(DnsUtilsLinux);

  for (int ifindex : m_linkDomains.keys()) {
    QList<QVariant> argumentList;
    argumentList << QVariant::fromValue(ifindex);
    argumentList << QVariant::fromValue(m_linkDomains[ifindex]);
    m_resolver->asyncCallWithArgumentList(QStringLiteral("SetLinkDomains"),
                                          argumentList);
  }

  if (m_ifindex > 0) {
    m_resolver->asyncCall(QStringLiteral("RevertLink"), m_ifindex);
  }

  logger.debug() << "DnsUtilsLinux destroyed.";
}

bool DnsUtilsLinux::updateResolvers(const QString& ifname,
                                    const QList<QHostAddress>& resolvers) {
  m_ifindex = if_nametoindex(qPrintable(ifname));
  if (m_ifindex <= 0) {
    logger.error() << "Unable to resolve ifindex for" << ifname;
    return false;
  }

  setLinkDNS(m_ifindex, resolvers);
  setLinkDefaultRoute(m_ifindex, true);
  updateLinkDomains();
  return true;
}

bool DnsUtilsLinux::restoreResolvers() {
  for (auto ifindex : m_linkDomains.keys()) {
    setLinkDomains(ifindex, m_linkDomains[ifindex]);
  }
  m_linkDomains.clear();

  /* Revert the VPN interface's DNS configuration */
  if (m_ifindex > 0) {
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
    logger.error() << "Error received from the DBus service";
  }
  delete call;
}

void DnsUtilsLinux::setLinkDNS(int ifindex,
                               const QList<QHostAddress>& resolvers) {
  QList<DnsResolver> resolverList;
  char ifnamebuf[IF_NAMESIZE];
  const char* ifname = if_indextoname(ifindex, ifnamebuf);
  for (auto ip : resolvers) {
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
  char ifnamebuf[IF_NAMESIZE];
  const char* ifname = if_indextoname(ifindex, ifnamebuf);
  if (ifname) {
    for (auto d : domains) {
      logger.debug() << "Setting DNS domain:" << d.domain << "via" << ifname
                     << (d.search ? "search" : "");
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
  if (reply.isError()) {
    logger.error() << "Error retrieving the DNS  domains from the DBus service";
    delete call;
    return;
  }

  /* Update the state of the DNS domains */
  m_linkDomains.clear();
  QDBusArgument args = qvariant_cast<QDBusArgument>(reply.value());
  QList<DnsDomain> list = qdbus_cast<QList<DnsDomain>>(args);
  for (auto d : list) {
    if (d.ifindex == 0) {
      continue;
    }
    m_linkDomains[d.ifindex].append(DnsLinkDomain(d.domain, d.search));
  }

  /* Drop any competing root search domains. */
  DnsLinkDomain root = DnsLinkDomain(".", true);
  for (auto ifindex : m_linkDomains.keys()) {
    if (!m_linkDomains[ifindex].contains(root)) {
      continue;
    }
    QList<DnsLinkDomain> newlist = m_linkDomains[ifindex];
    newlist.removeAll(root);
    setLinkDomains(ifindex, newlist);
  }

  /* Add a root search domain for the new interface. */
  QList<DnsLinkDomain> newlist = {root};
  setLinkDomains(m_ifindex, newlist);
  delete call;
}

static DnsMetatypeRegistrationProxy s_dnsMetatypeProxy;
