/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "cryptosettings.h"

void CryptoSettings::resetKey() {}

bool CryptoSettings::getKey(uint8_t key[CRYPTO_SETTINGS_KEY_SIZE]) {
  Q_UNUSED(key);
  return false;
}

// static
CryptoSettings::Version CryptoSettings::getSupportedVersion() {
  return CryptoSettings::NoEncryption;
}
