#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QRandomGenerator>
#include <QString>

#include "3rd/QJsonStruct/QJsonIO.hpp"
#include "serialization.h"
#include "transfer.h"

namespace amnezia::serialization::inbounds
{

//"inbounds": [
//                 {
//                     "listen": "127.0.0.1",
//                     "port": 10808,
//                     "protocol": "socks",
//                     "settings": {
//                         "udp": true,
//                         "auth": "password",
//                         "accounts": [ { "user": "<random>", "pass": "<random>" } ]
//                     }
//                 }
//],
//
// The local SOCKS5 inbound is an in-process IPC channel between the
// Xray core and the tun2socks component. On Android the loopback
// interface is shared across UIDs, so without authentication any
// third-party app with INTERNET permission can use it as an open
// proxy and bypass per-app split tunneling (see #2452).
//
// We require SOCKS5 username/password authentication. Credentials
// are generated once per process lifetime via QRandomGenerator::system()
// and are never persisted to disk or logs. The tun2socks consumer
// must read them via GetInboundCredentials() and feed them into its
// own upstream SOCKS5 client config.

const static QString listen = "127.0.0.1";
const static int port = 10808;
const static QString protocol = "socks";

namespace
{
    QString RandomToken(int length = 24)
    {
        static const char alphabet[] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789";
        QString out;
        out.reserve(length);
        auto *rng = QRandomGenerator::system();
        for (int i = 0; i < length; ++i) {
            out.append(QLatin1Char(alphabet[rng->bounded(int(sizeof(alphabet) - 1))]));
        }
        return out;
    }

    const InboundCredentials &Credentials()
    {
        static const InboundCredentials c{ listen, port, RandomToken(), RandomToken() };
        return c;
    }
}

InboundCredentials GetInboundCredentials()
{
    return Credentials();
}

QJsonObject GenerateInboundEntry()
{
    const auto &c = Credentials();

    QJsonObject account;
    account.insert("user", c.user);
    account.insert("pass", c.pass);

    QJsonArray accounts;
    accounts.append(account);

    QJsonObject root;
    QJsonIO::SetValue(root, listen, "listen");
    QJsonIO::SetValue(root, port, "port");
    QJsonIO::SetValue(root, protocol, "protocol");
    QJsonIO::SetValue(root, true, "settings", "udp");
    QJsonIO::SetValue(root, QString("password"), "settings", "auth");
    QJsonIO::SetValue(root, accounts, "settings", "accounts");
    return root;
}


} // namespace amnezia::serialization::inbounds
