// Copyright (c) Qv2ray, A Qt frontend for V2Ray. Written in C++.
// This file is part of the Qv2ray VPN client.
//
// Qv2ray, A Qt frontend for V2Ray. Written in C++

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

// Copyright (c) 2024 AmneziaVPN
// This file has been modified for AmneziaVPN
//
// This file is based on the work of the Qv2ray VPN client.
// The original code of the Qv2ray, A Qt frontend for V2Ray. Written in C++ and licensed under GPL3.
//
// The modified version of this file is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this file. If not, see <https://www.gnu.org/licenses/>.


#include "3rd/QJsonStruct/QJsonIO.hpp"
#include <QUrlQuery>
#include "serialization.h"

namespace amnezia::serialization::vless
{
QJsonObject Deserialize(const QString &str, QString *alias, QString *errMessage)
{
    // must start with vless://
    if (!str.startsWith("vless://"))
    {
        *errMessage = QObject::tr("VLESS link should start with vless://");
        return QJsonObject();
    }

    // parse url
    QUrl url(str);
    if (!url.isValid())
    {
        *errMessage = QObject::tr("link parse failed: %1").arg(url.errorString());
        return QJsonObject();
    }

    // fetch host
    const auto hostRaw = url.host();
    if (hostRaw.isEmpty())
    {
        *errMessage = QObject::tr("empty host");
        return QJsonObject();
    }
    const auto host = (hostRaw.startsWith('[') && hostRaw.endsWith(']')) ? hostRaw.mid(1, hostRaw.length() - 2) : hostRaw;

    // fetch port
    const auto port = url.port();
    if (port == -1)
    {
        *errMessage = QObject::tr("missing port");
        return QJsonObject();
    }

    // fetch remarks
    const auto remarks = url.fragment();
    if (!remarks.isEmpty())
    {
        *alias = remarks;
    }

    // fetch uuid
    const auto uuid = url.userInfo();
    if (uuid.isEmpty())
    {
        *errMessage = QObject::tr("missing uuid");
        return QJsonObject();
    }

    // initialize QJsonObject with basic info
    QJsonObject outbound;
    QJsonObject stream;

    QJsonIO::SetValue(outbound, "vless", "protocol");
    QJsonIO::SetValue(outbound, host, { "settings", "vnext", 0, "address" });
    QJsonIO::SetValue(outbound, port, { "settings", "vnext", 0, "port" });
    QJsonIO::SetValue(outbound, uuid, { "settings", "vnext", 0, "users", 0, "id" });

    // parse query
    QUrlQuery query(url.query());

    // handle type
    const auto hasType = query.hasQueryItem("type");
    const auto type = hasType ? query.queryItemValue("type") : "tcp";
    if (type != "tcp")
        QJsonIO::SetValue(stream, type, "network");

    // handle encryption
    const auto hasEncryption = query.hasQueryItem("encryption");
    const auto encryption = hasEncryption ? query.queryItemValue("encryption") : "none";
    QJsonIO::SetValue(outbound, encryption, { "settings", "vnext", 0, "users", 0, "encryption" });

    // type-wise settings
    if (type == "http" || type == "h2" || type == "h3" || type == "quic")
    {
        *errMessage = QObject::tr("transport \"%1\" was removed from Xray-core, use XHTTP").arg(type);
        return QJsonObject();
    }
    if (query.queryItemValue("security") == "xtls")
    {
        *errMessage = QObject::tr("security \"xtls\" was removed from Xray-core, use TLS or REALITY with xtls-rprx-vision");
        return QJsonObject();
    }
    if (type == "kcp")
    {
        const auto headerType = query.hasQueryItem("headerType") ? query.queryItemValue("headerType") : "none";
        if (query.hasQueryItem("seed") || headerType != "none")
        {
            *errMessage = QObject::tr("mKCP seed and header were removed from Xray-core");
            return QJsonObject();
        }
    }
    else if (type == "xhttp" || type == "splithttp")
    {
        if (query.hasQueryItem("host"))
            QJsonIO::SetValue(stream, query.queryItemValue("host"), { "xhttpSettings", "host" });
        if (query.hasQueryItem("path"))
            QJsonIO::SetValue(stream, QUrl::fromPercentEncoding(query.queryItemValue("path").toUtf8()), { "xhttpSettings", "path" });
        if (query.hasQueryItem("mode"))
            QJsonIO::SetValue(stream, query.queryItemValue("mode"), { "xhttpSettings", "mode" });
        if (query.hasQueryItem("extra"))
        {
            const auto extra = QJsonDocument::fromJson(QUrl::fromPercentEncoding(query.queryItemValue("extra").toUtf8()).toUtf8());
            if (extra.isObject())
                QJsonIO::SetValue(stream, extra.object(), { "xhttpSettings", "extra" });
        }
    }
    else if (type == "httpupgrade")
    {
        if (query.hasQueryItem("host"))
            QJsonIO::SetValue(stream, query.queryItemValue("host"), { "httpupgradeSettings", "host" });
        if (query.hasQueryItem("path"))
            QJsonIO::SetValue(stream, QUrl::fromPercentEncoding(query.queryItemValue("path").toUtf8()), { "httpupgradeSettings", "path" });
    }
    else if (type == "ws")
    {
        const auto hasPath = query.hasQueryItem("path");
        const auto path = hasPath ? QUrl::fromPercentEncoding(query.queryItemValue("path").toUtf8()) : "/";
        if (path != "/")
            QJsonIO::SetValue(stream, path, { "wsSettings", "path" });

        const auto hasHost = query.hasQueryItem("host");
        if (hasHost)
        {
            QJsonIO::SetValue(stream, query.queryItemValue("host"), { "wsSettings", "host" });
        }
    }
    else if (type == "grpc")
    {
        const auto hasServiceName = query.hasQueryItem("serviceName");
        if (hasServiceName)
        {
            const auto serviceName = QUrl::fromPercentEncoding(query.queryItemValue("serviceName").toUtf8());
            QJsonIO::SetValue(stream, serviceName, { "grpcSettings", "serviceName" });
        }

        const auto hasMode = query.hasQueryItem("mode");
        if (hasMode)
        {
            const auto multiMode = QUrl::fromPercentEncoding(query.queryItemValue("mode").toUtf8()) == "multi";
            QJsonIO::SetValue(stream, multiMode, { "grpcSettings", "multiMode" });
        }
    }

    // tls-wise settings
    const auto hasSecurity = query.hasQueryItem("security");
    const auto security = hasSecurity ? query.queryItemValue("security") : "none";
    const auto tlsKey = security == "tls" ? "tlsSettings" : "realitySettings";
    if (security != "none")
    {
        QJsonIO::SetValue(stream, security, "security");
    }
    // sni
    const auto hasSNI = query.hasQueryItem("sni");
    if (hasSNI)
    {
        const auto sni = query.queryItemValue("sni");
        QJsonIO::SetValue(stream, sni, { tlsKey, "serverName" });
    }
    // alpn
    const auto hasALPN = query.hasQueryItem("alpn");
    if (hasALPN)
    {
        const auto alpnRaw = QUrl::fromPercentEncoding(query.queryItemValue("alpn").toUtf8());
        QStringList aplnElems = alpnRaw.split(",");
        if (!aplnElems.isEmpty()) {
            const auto alpnArray = QJsonArray::fromStringList(aplnElems);
            QJsonIO::SetValue(stream, alpnArray, { tlsKey, "alpn" });
        }
    }
    if (security == "tls" || security == "reality")
    {
        const auto flow = query.queryItemValue("flow");
        QJsonIO::SetValue(outbound, flow, { "settings", "vnext", 0, "users", 0, "flow" });
        if (query.hasQueryItem("fp"))
        {
            const auto fp = QUrl::fromPercentEncoding(query.queryItemValue("fp").toUtf8());
            QJsonIO::SetValue(stream, fp, { tlsKey, "fingerprint" });
        }
    }
    if (security == "tls" && query.hasQueryItem("pcs"))
    {
        QJsonIO::SetValue(stream, query.queryItemValue("pcs"), { "tlsSettings", "pinnedPeerCertSha256" });
    }

    if (security == "reality")
    {
        if (query.hasQueryItem("pbk"))
        {
            const auto pbk = QUrl::fromPercentEncoding(query.queryItemValue("pbk").toUtf8());
            QJsonIO::SetValue(stream, pbk, { "realitySettings", "publicKey" });
        }
        const auto spiderXKey = query.hasQueryItem("spx") ? "spx" : "spiderX";
        if (query.hasQueryItem(spiderXKey))
        {
            const auto spiderX = QUrl::fromPercentEncoding(query.queryItemValue(spiderXKey).toUtf8());
            QJsonIO::SetValue(stream, spiderX, { "realitySettings", "spiderX" });
        }
        if (query.hasQueryItem("sid"))
        {
            const auto sid = QUrl::fromPercentEncoding(query.queryItemValue("sid").toUtf8());
            QJsonIO::SetValue(stream, sid, { "realitySettings", "shortId" });
        }
    }

    // assembling config
    QJsonObject root;
    outbound["streamSettings"] = stream;
    QJsonObject inbound = inbounds::GenerateInboundEntry();
    root["outbounds"] = QJsonArray{ outbound };
    root["inbounds"] = QJsonArray { inbound };
    return root;
}

const QString Serialize(const VlessServerObject &server, const QString &alias)
{
    
    QUrl url;
    
    // Set basic URL components
    url.setScheme("vless");
    url.setUserInfo(server.id);
    url.setHost(server.address);
    url.setPort(server.port);

    QUrlQuery query;

    if (!server.network.isEmpty() && server.network != "tcp") {
        query.addQueryItem("type", server.network);
    }
    
    if (!server.encryption.isEmpty()) {
        query.addQueryItem("encryption", server.encryption);
    }
    
    if (!server.security.isEmpty() && server.security != "none") {
        query.addQueryItem("security", server.security);
    }

    if (!server.flow.isEmpty() && (server.security == "reality" || server.security == "tls")) {
        query.addQueryItem("flow", server.flow);
    }

    if (!server.serverName.isEmpty()) {
        query.addQueryItem("sni", server.serverName);
    }

    if (!server.fingerprint.isEmpty() && (server.security == "reality" || server.security == "tls")) {
        query.addQueryItem("fp", server.fingerprint);
    }

    if (!server.alpn.isEmpty() && server.security == "tls") {
        query.addQueryItem("alpn", server.alpn);
    }

    if (server.network == "xhttp") {
        if (!server.xhttpHost.isEmpty()) {
            query.addQueryItem("host", server.xhttpHost);
        }
        if (!server.xhttpPath.isEmpty()) {
            query.addQueryItem("path", server.xhttpPath);
        }
        if (!server.xhttpMode.isEmpty()) {
            query.addQueryItem("mode", server.xhttpMode);
        }
        if (!server.xhttpExtra.isEmpty()) {
            query.addQueryItem("extra", server.xhttpExtra);
        }
    }

    if (!server.tlsPin.isEmpty() && server.security == "tls") {
        query.addQueryItem("pcs", server.tlsPin);
    }

    if (server.security == "reality") {
        if (!server.publicKey.isEmpty()) {
            query.addQueryItem("pbk", server.publicKey);
        }
        
        if (!server.shortId.isEmpty()) {
            query.addQueryItem("sid", server.shortId);
        }
        
        if (!server.spiderX.isEmpty()) {
            query.addQueryItem("spx", server.spiderX);
        }
    }
    
    url.setQuery(query);
    
    if (!alias.isEmpty()) {
        url.setFragment(alias);
    }
    
    return url.toString(QUrl::ComponentFormattingOption::FullyEncoded);
}
}

