/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef IPUTILS_H
#define IPUTILS_H

#include <QCoreApplication>
#include <QObject>

#include "interfaceconfig.h"

class IPUtils : public QObject {
 public:
  explicit IPUtils(QObject* parent) : QObject(parent){};
  virtual ~IPUtils() = default;

  virtual bool addInterfaceIPs(const InterfaceConfig& config) {
    Q_UNUSED(config);
    qFatal("Have you forgotten to implement IPUtils::addInterfaceIPs?");
    return false;
  };

  virtual bool setMTUAndUp(const InterfaceConfig& config) {
    Q_UNUSED(config);
    qFatal("Have you forgotten to implement IPUtils::setMTUAndUp?");
    return false;
  };
};

#endif  // IPUTILS_H