#include <QString>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QHostAddress>
#include <QRandomGenerator>
#include <QTcpServer>
#include <stdexcept>
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

static int indexOfSocksInbound(const QJsonArray &inbounds)
{
    for (int i = 0; i < inbounds.size(); ++i) {
        const QString p = inbounds.at(i).toObject().value(QLatin1String("protocol")).toString();
        if (p.compare(QLatin1String("socks"), Qt::CaseInsensitive) == 0)
            return i;
    }
    return -1;
}

// Ask the OS for a free TCP port on loopback (same stack as inbound "listen": "127.0.0.1").
static int acquireFreeLocalPort()
{
    QTcpServer probe;
    if (!probe.listen(QHostAddress(QStringLiteral("127.0.0.1")), 0)) {
        throw std::runtime_error(
            "Failed to bind a local TCP port on 127.0.0.1 for SOCKS inbound "
            "(QTcpServer::listen failed; possible permission or OS network error).");
    }
    return static_cast<int>(probe.serverPort());
}

// Generates a hex string of `byteCount` random bytes (URL-safe, no special chars).
static QString generateRandomHex(int byteCount)
{
    if (byteCount <= 0)
        return {};
    // fillRange writes full quint32 words; size the buffer to a multiple of 4 bytes to avoid
    // overrunning a short buffer when byteCount is not divisible by 4.
    const int numUint32 = (byteCount + int(sizeof(quint32)) - 1) / int(sizeof(quint32));
    QByteArray buf(numUint32 * int(sizeof(quint32)), '\0');
    QRandomGenerator::system()->fillRange(reinterpret_cast<quint32 *>(buf.data()), numUint32);
    return QString::fromLatin1(buf.left(byteCount).toHex());
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
    const int socksIdx = indexOfSocksInbound(inbounds);
    if (socksIdx < 0)
        return creds;

    const QJsonObject inbound = inbounds.at(socksIdx).toObject();
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
    QJsonArray inbounds = xrayConfig.value("inbounds").toArray();
    const int socksIdx = indexOfSocksInbound(inbounds);
    if (socksIdx < 0)
        return GetInboundCredentials(xrayConfig); // no SOCKS inbound to patch

    QJsonObject inbound = inbounds.at(socksIdx).toObject();
    InboundCredentials creds;
    creds.port = acquireFreeLocalPort();
    inbound["port"] = creds.port;

    QJsonObject settings = inbound.value("settings").toObject();
    const QJsonArray accounts = settings.value("accounts").toArray();
    if (!accounts.isEmpty()) {
        const QJsonObject account = accounts.first().toObject();
        creds.username = account.value("user").toString();
        creds.password = account.value("pass").toString();
    }

    if (creds.username.isEmpty() || creds.password.isEmpty()) {
        // Generate fresh credentials for this session (never persisted)
        creds.username = generateRandomHex(8);  // 16 hex chars
        creds.password = generateRandomHex(16); // 32 hex chars
        QJsonObject account;
        account["user"] = creds.username;
        account["pass"] = creds.password;
        settings["accounts"] = QJsonArray{ account };
    }

    // Always ensure auth mode is enforced, even for imported configs that had
    // accounts but auth: "noauth" (or no auth field at all).
    settings["auth"] = QStringLiteral("password");
    inbound["settings"] = settings;
    inbounds[socksIdx] = inbound;
    xrayConfig["inbounds"] = inbounds;

    return creds;
}

} // namespace amnezia::serialization::inbounds

