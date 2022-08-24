#include "MobileUtils.h"

void MobileUtils::shareText(const QStringList&) {}

void MobileUtils::writeToKeychain(const QString&, const QByteArray &) {}
bool MobileUtils::deleteFromKeychain(const QString& tag) { return false; }
QByteArray MobileUtils::readFromKeychain(const QString&) { return {}; }
