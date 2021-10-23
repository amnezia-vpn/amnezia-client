/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "iosdatamigration.h"
#include "device.h"
#include "logger.h"
#include "mozillavpn.h"
#include "user.h"

#include <QByteArray>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#import <Foundation/Foundation.h>

namespace {
Logger logger(LOG_IOS, "IOSDataMigration");

void migrateUserDefaultData() {
  AmneziaVPN* vpn = AmneziaVPN::instance();
  Q_ASSERT(vpn);

  NSUserDefaults* sud = [NSUserDefaults standardUserDefaults];
  if (!sud) {
    return;
  }

  NSData* userData = [sud dataForKey:@"user"];
  if (userData) {
    QByteArray json = QByteArray::fromNSData(userData);
    if (!json.isEmpty()) {
      logger.debug() << "User data to be migrated";
      vpn->accountChecked(json);
    }
  }

  NSData* deviceData = [sud dataForKey:@"device"];
  if (deviceData) {
    QByteArray json = QByteArray::fromNSData(deviceData);
    logger.debug() << "Device data to be migrated";
    // Nothing has to be done here because the device data is part of the user data.
  }

  NSData* serversData = [sud dataForKey:@"vpnServers"];
  if (serversData) {
    QByteArray json = QByteArray::fromNSData(serversData);
    if (!json.isEmpty()) {
      logger.debug() << "Server list data to be migrated";

      // We need to wrap the server list in a object to make it similar to the REST API response.
      QJsonDocument serverList = QJsonDocument::fromJson(json);
      if (!serverList.isArray()) {
        logger.error() << "Server list should be an array!";
        return;
      }

      QJsonObject countriesObj;
      countriesObj.insert("countries", QJsonValue(serverList.array()));

      QJsonDocument doc;
      doc.setObject(countriesObj);
      if (!vpn->setServerList(doc.toJson())) {
        logger.error() << "Server list cannot be imported";
        return;
      }
    }
  }

  NSData* selectedCityData = [sud dataForKey:@"selectedCity"];
  if (selectedCityData) {
    QByteArray json = QByteArray::fromNSData(selectedCityData);
    logger.debug() << "SelectedCity data to be migrated" << json;
    // Nothing has to be done here because the device data is part of the user data.

    QJsonDocument doc = QJsonDocument::fromJson(json);
    if (!doc.isObject()) {
      logger.error() << "SelectedCity should be an object";
      return;
    }

    QJsonObject obj = doc.object();
    QJsonValue code = obj.value("flagCode");
    if (!code.isString()) {
      logger.error() << "SelectedCity code should be a string";
      return;
    }

    QJsonValue name = obj.value("code");
    if (!name.isString()) {
      logger.error() << "SelectedCity name should be a string";
      return;
    }

    ServerData serverData;
    if (vpn->serverCountryModel()->pickIfExists(code.toString(), name.toString(), serverData)) {
      logger.debug() << "ServerCity found";
      serverData.writeSettings();
    }
  }
}

void migrateKeychainData() {
  NSData* service = [@"org.mozilla.guardian.credentials" dataUsingEncoding:NSUTF8StringEncoding];

  NSMutableDictionary* query = [[NSMutableDictionary alloc] init];

  [query setObject:(id)kSecClassGenericPassword forKey:(id)kSecClass];
  [query setObject:service forKey:(id)kSecAttrService];
  [query setObject:(id)kCFBooleanTrue forKey:(id)kSecReturnData];
  [query setObject:(id)kSecMatchLimitOne forKey:(id)kSecMatchLimit];

  NSData* dataNS = NULL;
  OSStatus status = SecItemCopyMatching((CFDictionaryRef)query, (CFTypeRef*)(void*)&dataNS);
  [query release];

  if (status != noErr) {
    logger.error() << "No credentials found";
    return;
  }

  QByteArray data = QByteArray::fromNSData(dataNS);
  logger.debug() << "Credentials:" << logger.sensitive(data);

  QJsonDocument json = QJsonDocument::fromJson(data);
  if (!json.isObject()) {
    logger.error() << "JSON object expected";
    return;
  }

  QJsonObject obj = json.object();
  QJsonValue deviceKeyValue = obj.value("deviceKeys");
  if (!deviceKeyValue.isObject()) {
    logger.error() << "JSON object should have a deviceKeys object";
    return;
  }

  QJsonObject deviceKeyObj = deviceKeyValue.toObject();
  QJsonValue publicKey = deviceKeyObj.value("publicKey");
  if (!publicKey.isString()) {
    logger.error() << "JSON deviceKey object should contain a publicKey value as string";
    return;
  }

  QJsonValue privateKey = deviceKeyObj.value("privateKey");
  if (!privateKey.isString()) {
    logger.error() << "JSON deviceKey object should contain a privateKey value as string";
    return;
  }

  QJsonValue token = obj.value("verificationToken");
  if (!token.isString()) {
    logger.error() << "JSON object should contain a verificationToken value s string";
    return;
  }

  AmneziaVPN::instance()->deviceAdded(Device::currentDeviceName(), publicKey.toString(),
                                      privateKey.toString());

  AmneziaVPN::instance()->setToken(token.toString());
}
}

// static
void IOSDataMigration::migrate() {
  logger.debug() << "IOS Data Migration";

  migrateKeychainData();
  migrateUserDefaultData();
}
