/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef IOSAUTHENTICATIONLISTENER_H
#define IOSAUTHENTICATIONLISTENER_H

#include "authenticationlistener.h"

#include <QObject>

class IOSAuthenticationListener final : public AuthenticationListener {
  Q_DISABLE_COPY_MOVE(IOSAuthenticationListener)

 public:
  IOSAuthenticationListener(QObject* parent);
  ~IOSAuthenticationListener();

  void start(const QString& codeChallenge, const QString& codeChallengeMethod,
             const QString& emailAddress) override;
};

#endif  // IOSAUTHENTICATIONLISTENER_H
