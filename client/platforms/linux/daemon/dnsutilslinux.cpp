/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "dnsutilslinux.h"

#include <net/if.h>

#include <QDBusVariant>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QSaveFile>
#include <QStandardPaths>
#include <QtDBus/QtDBus>

#include "leakdetector.h"
#include "logger.h"

constexpr const char* DBUS_RESOLVE_SERVICE = "org.freedesktop.resolve1";
constexpr const char* DBUS_RESOLVE_PATH = "/org/freedesktop/resolve1";
constexpr const char* DBUS_RESOLVE_MANAGER = "org.freedesktop.resolve1.Manager";
constexpr const char* DBUS_PROPERTY_INTERFACE =
    "org.freedesktop.DBus.Properties";

constexpr const char* RESOLV_CONF_PATH = "/etc/resolv.conf";
constexpr const char* RESOLVCONF_TAG = "amneziavpn";
constexpr const char* FALLBACK_STATE_DIR = "/var/lib/amneziavpn";

namespace {
Logger logger("DnsUtilsLinux");
}

DnsUtilsLinux::DnsUtilsLinux(QObject* parent) : DnsUtils(parent) {
  MZ_COUNT_CTOR(DnsUtilsLinux);
  logger.debug() << "DnsUtilsLinux created.";

  QDBusConnection conn = QDBusConnection::systemBus();
  m_resolver = new QDBusInterface(DBUS_RESOLVE_SERVICE, DBUS_RESOLVE_PATH,
                                  DBUS_RESOLVE_MANAGER, conn, this);
}

DnsUtilsLinux::~DnsUtilsLinux() {
  MZ_COUNT_DTOR(DnsUtilsLinux);

  if (m_fallbackActive) {
    restoreFallbackResolvers();
  } else {
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
  m_ifindex = if_nametoindex(qPrintable(ifname));
  if (m_ifindex <= 0) {
    logger.error() << "Unable to resolve ifindex for" << ifname;
    return false;
  }

  if (!resolvedAvailable()) {
    logger.info() << "org.freedesktop.resolve1 is unavailable, using the "
                     "resolvconf or /etc/resolv.conf fallback";
    m_fallback = true;
    return updateResolversFallback(resolvers);
  }

  m_fallback = false;
  setLinkDNS(m_ifindex, resolvers);
  setLinkDefaultRoute(m_ifindex, true);
  updateLinkDomains();
  return true;
}

bool DnsUtilsLinux::restoreResolvers() {
  if (m_fallback || m_fallbackActive) {
    const bool ok = restoreFallbackResolvers();
    m_fallback = false;
    m_ifindex = 0;
    return ok;
  }

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

  return true;
}

bool DnsUtilsLinux::resolvedAvailable() const {
  QDBusConnection conn = QDBusConnection::systemBus();
  if (!conn.isConnected()) {
    return false;
  }

  QDBusConnectionInterface* iface = conn.interface();
  if (!iface) {
    return false;
  }

  QDBusReply<bool> reply = iface->isServiceRegistered(DBUS_RESOLVE_SERVICE);
  return reply.isValid() && reply.value();
}

QString DnsUtilsLinux::findResolvconf() const {
  const QStringList candidates = {
      QStringLiteral("/sbin/resolvconf"),
      QStringLiteral("/usr/sbin/resolvconf"),
      QStringLiteral("/usr/bin/resolvconf"),
      QStringLiteral("/bin/resolvconf"),
  };
  for (const QString& path : candidates) {
    if (QFileInfo(path).isExecutable()) {
      return path;
    }
  }
  return QStandardPaths::findExecutable(QStringLiteral("resolvconf"));
}

bool DnsUtilsLinux::runResolvconf(const QString& program,
                                  const QStringList& args,
                                  const QByteArray& input) {
  QProcess process;
  process.setProcessChannelMode(QProcess::MergedChannels);
  process.start(program, args);
  if (!process.waitForStarted(3000)) {
    logger.warning() << "Unable to start" << program;
    return false;
  }

  if (!input.isEmpty()) {
    process.write(input);
  }
  process.closeWriteChannel();

  if (!process.waitForFinished(5000)) {
    process.kill();
    process.waitForFinished(1000);
    logger.warning() << program << args << "timed out";
    return false;
  }

  if (process.exitStatus() != QProcess::NormalExit ||
      process.exitCode() != 0) {
    logger.warning() << program << args << "failed:"
                     << QString::fromUtf8(process.readAll()).trimmed();
    return false;
  }

  return true;
}

bool DnsUtilsLinux::updateResolversFallback(
    const QList<QHostAddress>& resolvers) {
  QByteArray content;
  for (const QHostAddress& address : resolvers) {
    content += "nameserver ";
    content += address.toString().toUtf8();
    content += "\n";
  }

  if (content.isEmpty()) {
    logger.error() << "No DNS resolvers supplied for the fallback";
    return false;
  }

  const QString resolvconf = findResolvconf();
  if (!resolvconf.isEmpty()) {
    const bool exclusive =
        runResolvconf(resolvconf,
                      {QStringLiteral("-a"), QString::fromLatin1(RESOLVCONF_TAG),
                       QStringLiteral("-x"), QStringLiteral("-m"),
                       QStringLiteral("0")},
                      content);
    if (exclusive ||
        runResolvconf(resolvconf,
                      {QStringLiteral("-a"), QString::fromLatin1(RESOLVCONF_TAG)},
                      content)) {
      m_fallbackActive = true;
      m_fallbackBackend = QStringLiteral("resolvconf");
      logger.info() << "DNS configured through resolvconf";
      return true;
    }
  }

  if (!writeResolvConf(content)) {
    return false;
  }

  m_fallbackActive = true;
  m_fallbackBackend = QStringLiteral("file");
  logger.info() << "DNS configured directly in" << RESOLV_CONF_PATH;
  return true;
}

bool DnsUtilsLinux::writeResolvConf(const QByteArray& content) {
  QDir stateDir(QString::fromLatin1(FALLBACK_STATE_DIR));
  if (!stateDir.mkpath(QStringLiteral("."))) {
    logger.error() << "Unable to create" << stateDir.absolutePath();
    return false;
  }

  const QString metaPath =
      stateDir.filePath(QStringLiteral("resolv.conf.meta"));
  const QString backupPath =
      stateDir.filePath(QStringLiteral("resolv.conf.bak"));
  const QFileInfo resolvConfInfo(QString::fromLatin1(RESOLV_CONF_PATH));

  QFile meta(metaPath);
  if (!meta.exists()) {
    QString kind = QStringLiteral("file");
    if (resolvConfInfo.isSymLink()) {
      kind = QStringLiteral("symlink:") + resolvConfInfo.symLinkTarget();
    }

    if (!meta.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
      logger.error() << "Unable to write" << metaPath;
      return false;
    }
    meta.write(kind.toUtf8() + '\n');
    meta.close();

    QFile::remove(backupPath);
    if (resolvConfInfo.exists()) {
      if (!QFile::copy(QString::fromLatin1(RESOLV_CONF_PATH), backupPath)) {
        QFile::remove(metaPath);
        logger.error() << "Unable to back up" << RESOLV_CONF_PATH;
        return false;
      }
    } else {
      QFile empty(backupPath);
      if (!empty.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QFile::remove(metaPath);
        logger.error() << "Unable to create" << backupPath;
        return false;
      }
    }
  }

  // A symlink would send our writes to a file owned by another daemon.
  if (resolvConfInfo.isSymLink()) {
    QFile::remove(QString::fromLatin1(RESOLV_CONF_PATH));
  }

  QSaveFile saveFile(QString::fromLatin1(RESOLV_CONF_PATH));
  if (saveFile.open(QIODevice::WriteOnly | QIODevice::Truncate) &&
      saveFile.write(content) == content.size() && saveFile.commit()) {
    // done
  } else {
    // /etc/resolv.conf can be a bind mount (containers, immutable images),
    // where rename() is not permitted.  Fall back to an in-place write.
    QFile direct(QString::fromLatin1(RESOLV_CONF_PATH));
    if (!direct.open(QIODevice::WriteOnly | QIODevice::Truncate) ||
        direct.write(content) != content.size()) {
      logger.error() << "Unable to write" << RESOLV_CONF_PATH;
      return false;
    }
  }

  QFile::setPermissions(QString::fromLatin1(RESOLV_CONF_PATH),
                        QFileDevice::ReadOwner | QFileDevice::WriteOwner |
                            QFileDevice::ReadGroup | QFileDevice::ReadOther);
  return true;
}

bool DnsUtilsLinux::restoreFallbackResolvers() {
  if (!m_fallbackActive) {
    return true;
  }

  bool ok = true;
  if (m_fallbackBackend == QStringLiteral("resolvconf")) {
    const QString resolvconf = findResolvconf();
    ok = !resolvconf.isEmpty() &&
         runResolvconf(resolvconf,
                       {QStringLiteral("-d"), QString::fromLatin1(RESOLVCONF_TAG)},
                       {});
  } else {
    const QString stateDir = QString::fromLatin1(FALLBACK_STATE_DIR);
    const QString metaPath = stateDir + QStringLiteral("/resolv.conf.meta");
    const QString backupPath = stateDir + QStringLiteral("/resolv.conf.bak");

    QFile meta(metaPath);
    if (!meta.open(QIODevice::ReadOnly)) {
      logger.error() << "Unable to read" << metaPath;
      ok = false;
    } else {
      const QString kind = QString::fromUtf8(meta.readAll()).trimmed();
      meta.close();

      if (kind.startsWith(QStringLiteral("symlink:"))) {
        const QString target = kind.mid(QStringLiteral("symlink:").size());
        QFile::remove(QString::fromLatin1(RESOLV_CONF_PATH));
        if (!QFile::link(target, QString::fromLatin1(RESOLV_CONF_PATH))) {
          logger.error() << "Unable to restore the" << RESOLV_CONF_PATH
                         << "symlink";
          ok = false;
        }
      } else if (!QFile::exists(backupPath)) {
        logger.error() << "DNS backup is missing:" << backupPath;
        ok = false;
      } else {
        QFile::remove(QString::fromLatin1(RESOLV_CONF_PATH));
        if (!QFile::copy(backupPath, QString::fromLatin1(RESOLV_CONF_PATH))) {
          logger.error() << "Unable to restore" << RESOLV_CONF_PATH;
          ok = false;
        }
      }

      QFile::remove(metaPath);
      QFile::remove(backupPath);
    }
  }

  m_fallbackActive = false;
  m_fallbackBackend.clear();
  return ok;
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
  if (m_fallback) {
    return;
  }

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
  if (m_fallback) {
    return;
  }

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
  if (m_fallback) {
    return;
  }

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
  if (m_fallback) {
    return;
  }

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
