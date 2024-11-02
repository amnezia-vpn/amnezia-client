#ifndef ENDPOINTS_H
#define ENDPOINTS_H
#include <QString>

const QString LOGIN_ENDPOINT = "/api/v1/auth/login";
const QString REGISTER_ENDPOINT = "/api/v1/auth/register";
const QString REFRESH_ENDPOINT = "/api/v1/auth/refresh";
const QString RECOVERY_ENDPOINT = "/api/v1/auth/recover";
const QString PASSWORD_CHANGE_ENDPOINT = "/api/v1/me/password";
const QString EMAIL_CHANGE_ENDPOINT = "/api/v1/me/email";
const QString ME_ENDPOINT = "/api/v1/me";
const QString SERVERS_ENDPOINT = "/api/v1/vpn/regions";
const QString CONNECTION_STRING_ENDPOINT = "/api/v1/vpn/connect";
const QString PAYMENT_ENDPOINT = "/api/v1/payments/create";
const QString ACTIVATE_PROMOCODE_ENDPOINT = "/api/v1/me/promocode";

const QString API_COMPAT_ENDPOINT = "/api/versions";
const int API_VERSION = 1;
const uint32_t BREAKING_HASH = 0xdeadbeef;

#ifdef Q_OS_MACOS
const QString MAC_UPDATE_ENDPOINT = "/api/v1/updates/mac/appcast.xml";
#endif

#endif // ENDPOINTS_H
