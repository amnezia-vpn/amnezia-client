/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DNSUTILS_H
#define DNSUTILS_H

#include <QHostAddress>
#include <QString>

#include "dnsutils.h"

class DnsUtils : public QObject {
  Q_OBJECT

 public:
  explicit DnsUtils(QObject* parent) : QObject(parent){};
  virtual ~DnsUtils() = default;

  virtual bool updateResolvers(const QString& ifname,
                               const QList<QHostAddress>& resolvers) {
    Q_UNUSED(ifname);
    Q_UNUSED(resolvers);
    qFatal("Have you forgotten to implement DnsUtils::updateResolvers?");
    return false;
  };

  virtual bool restoreResolvers() {
    qFatal("Have you forgotten to implement DnsUtils::restoreResolvers?");
    return false;
  }
};

#endif  // DNSUTILS_H
