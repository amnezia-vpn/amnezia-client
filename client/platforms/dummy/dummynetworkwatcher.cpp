/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "dummynetworkwatcher.h"

#include "leakdetector.h"

DummyNetworkWatcher::DummyNetworkWatcher(QObject* parent)
    : NetworkWatcherImpl(parent) {
  MZ_COUNT_CTOR(DummyNetworkWatcher);
}

DummyNetworkWatcher::~DummyNetworkWatcher() {
  MZ_COUNT_DTOR(DummyNetworkWatcher);
}

void DummyNetworkWatcher::initialize() {}
