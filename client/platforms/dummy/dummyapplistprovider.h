/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DUMMYAPPLISTPROVIDER_H
#define DUMMYAPPLISTPROVIDER_H

#include <QObject>

#include "applistprovider.h"

class DummyAppListProvider : public AppListProvider {
  Q_OBJECT
 public:
  DummyAppListProvider(QObject* parent);
  ~DummyAppListProvider();
  void getApplicationList() override;
};

#endif  // DUMMYAPPLISTPROVIDER_H
