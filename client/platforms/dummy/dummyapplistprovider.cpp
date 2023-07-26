/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "dummyapplistprovider.h"

#include "leakdetector.h"

DummyAppListProvider::DummyAppListProvider(QObject* parent)
    : AppListProvider(parent) {
  MZ_COUNT_CTOR(DummyAppListProvider);
}

DummyAppListProvider::~DummyAppListProvider() {
  MZ_COUNT_DTOR(DummyAppListProvider);
}

void DummyAppListProvider::getApplicationList() {
  QMap<QString, QString> appList;
  appList["com.example.one"] = "a Example App 1";
  appList["com.example.two"] = "B Example App 2";
  appList["org.example.one"] = "c Example App 3";
  appList["org.example.two"] = "D Example App 4";
  appList["com.example.a"] = "e Example App 5";
  appList["com.example.b"] = "F Example App 6";
  appList["org.example.c"] = "g Example App 7";
  appList["org.example.d"] = "H Example App 8";
  emit newAppList(appList);
}
