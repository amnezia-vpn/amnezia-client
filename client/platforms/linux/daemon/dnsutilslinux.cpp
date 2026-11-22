/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "dnsutilslinux.h"

#include <net/if.h>

#include <QDBusVariant>
#include <QFileInfo>
#include <QProcess>
#include <QtDBus/QtDBus>

#include "leakdetector.h"
#include "logger.h"

constexpr const char* DBUS_RESOLVE_SERVICE = "org.freedesktop.resolve1";
constexpr const char* DBUS_RESOLVE_PATH = "/org/freedesktop/resolve1";
constexpr const char* DBUS_RESOLVE_MANAGER = "org.freedesktop.resolve1.Manager";
constexpr const char* DBUS_PROPERTY_INTERFACE =
    "org.freedesktop.DBus.Properties";

namespace {
Logger logger("DnsUtilsLinux");

// Applications only consume the per-link configuration set through the
// resolved D-Bus API when they use the systemd-resolved stub resolver.
// Otherwise (e.g. a resolvconf-managed /etc/resolv.conf) the tunnel DNS has
// to be pushed into the system resolver as well.
bool isResolvedManagingResolvConf() {
  const QFileInfo info(QStringLiteral("/etc/resolv.conf"));
  if (!info.isSymLink()) {
    return false;
  }
  return info.symLinkTarget().contains(QStringLiteral("systemd/resolve"));
}

QString findResolvconfBinary() {
  const QStringList paths = {
      QStringLiteral("/usr/sbin/resolvconf"), QStringLiteral("/sbin/resolvconf"),
      QStringLiteral("/usr/bin/resolvconf"), QStringLiteral("/bin/resolvconf"),
  };
  for (const QString& path : paths) {
    if (QFileInfo::exists(path)) {
      return path;
    }
  }
  return QString();
}
}  // namespace

DnsUtilsLinux::DnsUtilsLinux(QObject* parent) : DnsUtils(parent) {
  MZ_COUNT_CTOR(DnsUtilsLinux);
  logger.debug() << "DnsUtilsLinux created.";

  QDBusConnection conn = QDBusConnection::systemBus();
  m_resolver = new QDBusInterface(DBUS_RESOLVE_SERVICE, DBUS_RESOLVE_PATH,
                                  DBUS_RESOLVE_MANAGER, conn, this);
}

DnsUtilsLinux::~DnsUtilsLinux() {
  MZ_COUNT_DTOR(DnsUtilsLinux);

  removeResolvconfEntry();

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

  if (isResolvedManagingResolvConf()) {
    m_resolvconfIface.clear();
  } else {
    // Push the tunnel resolvers into the system resolver too, the same way
    // the OpenVPN update-resolv-conf script does.
    m_resolvconfIface = ifname;
    updateResolvConf(resolvers);
  }
  return true;
}

bool DnsUtilsLinux::restoreResolvers() {
  for (auto iterator = m_linkDomains.constBegin();
       iterator != m_linkDomains.constEnd(); ++iterator) {
    setLinkDomains(iterator.key(), iterator.value());
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

  removeResolvconfEntry();

  return true;
}

void DnsUtilsLinux::updateResolvConf(const QList<QHostAddress>& resolvers) {
  const QString resolvconf = findResolvconfBinary();
  if (resolvconf.isEmpty()) {
    logger.warning()
        << "resolvconf binary not found, the system resolver is not updated";
    return;
  }

  QProcess process;
  process.start(resolvconf,
                {QStringLiteral("-a"), m_resolvconfIface + QStringLiteral(".inet")});
  if (!process.waitForStarted(3000)) {
    logger.error() << "Failed to start" << resolvconf;
    return;
  }

  QByteArray data;
  for (const QHostAddress& address : resolvers) {
    if (!address.isNull()) {
      data.append("nameserver " + address.toString().toUtf8() + "\n");
    }
  }
  process.write(data);
  process.closeWriteChannel();

  if (!process.waitForFinished(10000) ||
      process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
    logger.error() << "resolvconf exited with an error";
    return;
  }
  logger.debug() << "System resolver updated via resolvconf for"
                 << m_resolvconfIface;
}

void DnsUtilsLinux::removeResolvconfEntry() {
  if (m_resolvconfIface.isEmpty()) {
    return;
  }

  const QString resolvconf = findResolvconfBinary();
  if (!resolvconf.isEmpty()) {
    QProcess process;
    process.start(resolvconf,
                  {QStringLiteral("-d"), m_resolvconfIface + QStringLiteral(".inet")});
    if (!process.waitForFinished(10000)) {
      logger.error() << "Failed to remove the resolvconf entry";
    }
  }
  m_resolvconfIface.clear();
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
  QList<DnsLinkDomain> newlist = {root};
  setLinkDomains(m_ifindex, newlist);
  delete call;
}

static DnsMetatypeRegistrationProxy s_dnsMetatypeProxy;
