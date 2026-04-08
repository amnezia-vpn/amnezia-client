#include <QString>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QRandomGenerator>
#include "3rd/QJsonStruct/QJsonIO.hpp"
#include "transfer.h"
#include "serialization.h"

namespace amnezia::serialization::inbounds
{

//"inbounds": [
//                 {
//                     "listen": "127.0.0.1",
//                     "port": 10808,
//                     "protocol": "socks",
//                     "settings": {
//                         "auth": "password",
//                         "accounts": [{"user": "...", "pass": "..."}],
//                         "udp": true
//                     }
//                 }
//],

const static QString listen = "127.0.0.1";
const static int defaultPort = 10808;
const static QString protocol = "socks";

// Generates a hex string of `byteCount` random bytes (URL-safe, no special chars).
static QString generateRandomHex(int byteCount)
{
    QByteArray buf(byteCount, 0);
    QRandomGenerator::system()->fillRange(reinterpret_cast<quint32 *>(buf.data()),
                                          (byteCount + sizeof(quint32) - 1) / sizeof(quint32));
    return QString::fromLatin1(buf.toHex()).left(byteCount * 2);
}

QJsonObject GenerateInboundEntry()
{
    QJsonObject root;
    QJsonIO::SetValue(root, listen, "listen");
    QJsonIO::SetValue(root, defaultPort, "port");
    QJsonIO::SetValue(root, protocol, "protocol");
    QJsonIO::SetValue(root, true, "settings", "udp");
    return root;
}

InboundCredentials GetInboundCredentials(const QJsonObject &xrayConfig)
{
    InboundCredentials creds;
    creds.port = defaultPort;

    const QJsonArray inbounds = xrayConfig.value("inbounds").toArray();
    if (inbounds.isEmpty())
        return creds;

    const QJsonObject inbound = inbounds.first().toObject();
    creds.port = inbound.value("port").toInt(defaultPort);

    const QJsonObject settings = inbound.value("settings").toObject();
    const QJsonArray accounts = settings.value("accounts").toArray();
    if (accounts.isEmpty())
        return creds;

    const QJsonObject account = accounts.first().toObject();
    creds.username = account.value("user").toString();
    creds.password = account.value("pass").toString();
    return creds;
}

InboundCredentials EnsureInboundAuth(QJsonObject &xrayConfig)
{
    InboundCredentials creds = GetInboundCredentials(xrayConfig);
    if (!creds.username.isEmpty() && !creds.password.isEmpty())
        return creds; // already has valid auth — reuse it

    // Generate fresh credentials for this session (never persisted)
    creds.username = generateRandomHex(8);  // 16 hex chars
    creds.password = generateRandomHex(16); // 32 hex chars

    QJsonArray inbounds = xrayConfig.value("inbounds").toArray();
    if (inbounds.isEmpty())
        return creds;

    QJsonObject inbound = inbounds.first().toObject();
    creds.port = inbound.value("port").toInt(defaultPort);

    QJsonObject settings = inbound.value("settings").toObject();
    settings["auth"] = QStringLiteral("password");
    QJsonObject account;
    account["user"] = creds.username;
    account["pass"] = creds.password;
    settings["accounts"] = QJsonArray{ account };

    inbound["settings"] = settings;
    inbounds[0] = inbound;
    xrayConfig["inbounds"] = inbounds;

    return creds;
}

} // namespace amnezia::serialization::inbounds

