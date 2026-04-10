/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "dnsutilslinux.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>

#include "leakdetector.h"
#include "logger.h"

namespace {
Logger logger("DnsUtilsLinux");
}

DnsUtilsLinux::DnsUtilsLinux(QObject* parent) : DnsUtils(parent) {
  MZ_COUNT_CTOR(DnsUtilsLinux);
  logger.debug() << "DnsUtilsLinux created.";
}

DnsUtilsLinux::~DnsUtilsLinux() {
  MZ_COUNT_DTOR(DnsUtilsLinux);
  logger.debug() << "DnsUtilsLinux destroyed.";
}

void DnsUtilsLinux::writeResolvConf(const QList<QHostAddress>& resolvers) {
  if (resolvers.isEmpty()) return;

  static const QString kPath = QStringLiteral("/etc/resolv.conf");

  if (m_resolvConfOriginal.isEmpty()) {
    QFileInfo fi(kPath);
    if (fi.isSymLink()) {
      m_resolvConfOriginal = fi.symLinkTarget();
      logger.debug() << "Saved resolv.conf symlink target:"
                     << m_resolvConfOriginal;
      if (!m_stateFilePath.isEmpty()) {
        QFile sf(m_stateFilePath);
        if (sf.open(QIODevice::WriteOnly | QIODevice::Text))
          sf.write(m_resolvConfOriginal.toUtf8());
      }
    } else {
      QFile orig(kPath);
      if (orig.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QByteArray content = orig.readAll();
        orig.close();
        m_resolvConfOriginal = QStringLiteral("__file__");
        logger.debug() << "resolv.conf is a regular file; saved content for restore";
        if (!m_stateFilePath.isEmpty()) {
          QFile sf(m_stateFilePath);
          if (sf.open(QIODevice::WriteOnly | QIODevice::Text)) {
            sf.write("__file__:");
            sf.write(content);
          }
        }
      } else {
        m_resolvConfOriginal = QStringLiteral("__file__");
      }
    }
  }

  QFile::remove(kPath);
  QFile f(kPath);
  if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
    logger.warning() << "Failed to write" << kPath << ":" << f.errorString();
    if (m_resolvConfOriginal != QStringLiteral("__file__"))
      QFile::link(m_resolvConfOriginal, kPath);
    m_resolvConfOriginal.clear();
    if (!m_stateFilePath.isEmpty()) QFile::remove(m_stateFilePath);
    return;
  }
  for (const auto& r : resolvers) {
    if (r.protocol() == QAbstractSocket::IPv4Protocol)
      f.write(
          QStringLiteral("nameserver %1\n").arg(r.toString()).toUtf8());
  }
  f.close();
  logger.debug() << "Wrote resolv.conf with" << resolvers.size()
                 << "DNS servers";
}

void DnsUtilsLinux::restoreResolvConf() {
  if (m_resolvConfOriginal.isEmpty()) return;

  const QString original = m_resolvConfOriginal;
  m_resolvConfOriginal.clear();

  if (!m_stateFilePath.isEmpty())
    QFile::remove(m_stateFilePath);

  static const char* kPath = "/etc/resolv.conf";

  QFile::remove(kPath);

  if (original == QStringLiteral("__file__")) {
    if (!m_resolvConfSavedContent.isEmpty()) {
      QFile f(kPath);
      if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        f.write(m_resolvConfSavedContent.toUtf8());
        logger.debug() << "Restored resolv.conf from saved content";
      }
      m_resolvConfSavedContent.clear();
    } else if (QFile::exists(QStringLiteral("/run/systemd/resolve/stub-resolv.conf"))) {
      QFile::link(QStringLiteral("/run/systemd/resolve/stub-resolv.conf"), kPath);
      logger.debug() << "Restored resolv.conf symlink to stub-resolv.conf";
    }
  } else {
    QFile::link(original, kPath);
    logger.debug() << "Restored resolv.conf symlink to" << original;
  }
}

bool DnsUtilsLinux::updateResolvers(const QString& ifname,
                                    const QList<QHostAddress>& resolvers) {
  m_stateFilePath = QStringLiteral("/run/amnezia-dns-%1").arg(ifname);

  writeResolvConf(resolvers);
  return true;
}

bool DnsUtilsLinux::restoreResolvers() {

  if (m_resolvConfOriginal.isEmpty()) {
    QStringList candidates;
    if (!m_stateFilePath.isEmpty()) {
      candidates << m_stateFilePath;
    } else {

      QDir runDir(QStringLiteral("/run"));
      for (const QString& name : runDir.entryList(
               {QStringLiteral("amnezia-dns-*")}, QDir::Files)) {
        candidates << runDir.filePath(name);
      }
    }
    for (const QString& path : candidates) {
      QFile sf(path);
      if (sf.open(QIODevice::ReadOnly)) {
        QByteArray data = sf.readAll();
        sf.close();
        m_stateFilePath = path;
        if (data.startsWith("__file__:")) {
          m_resolvConfOriginal = QStringLiteral("__file__");
          m_resolvConfSavedContent = QString::fromUtf8(data.mid(9));
        } else {
          m_resolvConfOriginal = QString::fromUtf8(data).trimmed();
        }
        logger.debug() << "Recovered DNS original from" << path << ":"
                       << m_resolvConfOriginal;
        break;
      }
    }
  }

  const bool hadDnsState = !m_resolvConfOriginal.isEmpty();
  restoreResolvConf();

  if (hadDnsState && QFile::exists(QStringLiteral("/run/systemd/resolve"))) {
    QProcess::startDetached("systemctl", {"restart", "systemd-resolved"});
  }

  return true;
}
