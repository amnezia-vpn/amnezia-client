/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef IOSIAPHANDLER_H
#define IOSIAPHANDLER_H

#include "iaphandler.h"

class IOSIAPHandler final : public IAPHandler {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(IOSIAPHandler)

 public:
  explicit IOSIAPHandler(QObject* parent);
  ~IOSIAPHandler();

 public slots:
  void productRegistered(void* product);
  void processCompletedTransactions(const QStringList& ids);

 protected:
  void nativeRegisterProducts() override;
  void nativeStartSubscription(Product* product) override;

 private:
  void* m_delegate = nullptr;
};

#endif  // IOSIAPHANDLER_H
