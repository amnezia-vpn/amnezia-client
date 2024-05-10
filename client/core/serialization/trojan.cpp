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

#define OUTBOUND_TAG_PROXY "PROXY"

namespace amnezia::serialization::trojan
{

const QString Serialize(const TrojanObject &object, const QString &alias)
{

    QUrlQuery query;
    if (object.ignoreHostname)
        query.addQueryItem("allowInsecureHostname", "1");
    if (object.ignoreCertificate)
        query.addQueryItem("allowInsecureCertificate", "1");
    if (object.sessionTicket)
        query.addQueryItem("sessionTicket", "1");
    if (object.ignoreCertificate || object.ignoreHostname)
        query.addQueryItem("allowInsecure", "1");
    if (object.tcpFastOpen)
        query.addQueryItem("tfo", "1");

    if (!object.sni.isEmpty())
        query.addQueryItem("sni", object.sni);

    QUrl link;
    if (!object.password.isEmpty())
        link.setUserName(object.password, QUrl::DecodedMode);
    link.setPort(object.port);
    link.setHost(object.address);
    link.setFragment(alias);
    link.setQuery(query);
    link.setScheme("trojan");

    return link.toString(QUrl::FullyEncoded);
}

QJsonObject Deserialize(const QString &trojanUri, QString *alias, QString *errMessage)
{
    const QString prefix = "trojan://";
    if (!trojanUri.startsWith(prefix))
    {
        *errMessage = ("Invalid Trojan URI");
        return {};
    }
    //
    const auto trueList = QStringList{ "true", "1", "yes", "y" };
    const QUrl trojanUrl(trojanUri.trimmed());
    const QUrlQuery query(trojanUrl.query());
    *alias = trojanUrl.fragment(QUrl::FullyDecoded);

    auto getQueryValue = [&](const QString &key) {
        return query.queryItemValue(key, QUrl::FullyDecoded);
    };
    //
    TrojanObject result;
    result.address = trojanUrl.host();
    result.password = QUrl::fromPercentEncoding(trojanUrl.userInfo().toUtf8());
    result.port = trojanUrl.port();
    // process sni (and also "peer")
    if (query.hasQueryItem("sni"))
    {
        result.sni = getQueryValue("sni");
    }
    else if (query.hasQueryItem("peer"))
    {
        // This is evil and may be removed in a future version.
        qWarning() << "use of 'peer' in trojan url is deprecated";
        result.sni = getQueryValue("peer");
    }
    else
    {
        // Use the hostname
        result.sni = result.address;
    }


    //
    result.tcpFastOpen = trueList.contains(getQueryValue("tfo").toLower());
    result.sessionTicket = trueList.contains(getQueryValue("sessionTicket").toLower());
    //
    bool allowAllInsecure = trueList.contains(getQueryValue("allowInsecure").toLower());
    result.ignoreHostname = allowAllInsecure || trueList.contains(getQueryValue("allowInsecureHostname").toLower());
    result.ignoreCertificate = allowAllInsecure || trueList.contains(getQueryValue("allowInsecureCertificate").toLower());

    QJsonObject stream;
    // handle type
    const auto hasType = query.hasQueryItem("type");
    const auto type = hasType ? query.queryItemValue("type") : "tcp";
    if (type != "tcp")
        QJsonIO::SetValue(stream, type, "network");


    // type-wise settings
    if (type == "kcp")
    {
        const auto hasSeed = query.hasQueryItem("seed");
        if (hasSeed)
            QJsonIO::SetValue(stream, query.queryItemValue("seed"), { "kcpSettings", "seed" });

        const auto hasHeaderType = query.hasQueryItem("headerType");
        const auto headerType = hasHeaderType ? query.queryItemValue("headerType") : "none";
        if (headerType != "none")
            QJsonIO::SetValue(stream, headerType, { "kcpSettings", "header", "type" });
    }
    else if (type == "http")
    {
        const auto hasPath = query.hasQueryItem("path");
        const auto path = hasPath ? QUrl::fromPercentEncoding(query.queryItemValue("path").toUtf8()) : "/";
        if (path != "/")
            QJsonIO::SetValue(stream, path, { "httpSettings", "path" });

        const auto hasHost = query.hasQueryItem("host");
        if (hasHost)
        {
            const auto hosts = QJsonArray::fromStringList(query.queryItemValue("host").split(","));
            QJsonIO::SetValue(stream, hosts, { "httpSettings", "host" });
        }
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
            QJsonIO::SetValue(stream, query.queryItemValue("host"), { "wsSettings", "headers", "Host" });
        }
    }
    else if (type == "quic")
    {
        const auto hasQuicSecurity = query.hasQueryItem("quicSecurity");
        if (hasQuicSecurity)
        {
            const auto quicSecurity = query.queryItemValue("quicSecurity");
            QJsonIO::SetValue(stream, quicSecurity, { "quicSettings", "security" });

            if (quicSecurity != "none")
            {
                const auto key = query.queryItemValue("key");
                QJsonIO::SetValue(stream, key, { "quicSettings", "key" });
            }

            const auto hasHeaderType = query.hasQueryItem("headerType");
            const auto headerType = hasHeaderType ? query.queryItemValue("headerType") : "none";
            if (headerType != "none")
                QJsonIO::SetValue(stream, headerType, { "quicSettings", "header", "type" });
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
    const auto tlsKey = security == "xtls" ? "xtlsSettings" : ( security == "tls" ? "tlsSettings" : "realitySettings" );
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
        // h2 protocol is not supported by xray
        aplnElems.removeAll("h2");
        if (!aplnElems.isEmpty()) {
            const auto alpnArray = QJsonArray::fromStringList(aplnElems);
            QJsonIO::SetValue(stream, alpnArray, { tlsKey, "alpn" });
        }
    }

    if (security == "reality")
    {
        if (query.hasQueryItem("fp"))
        {
            const auto fp = QUrl::fromPercentEncoding(query.queryItemValue("fp").toUtf8());
            QJsonIO::SetValue(stream, fp, { "realitySettings", "fingerprint" });
        }
        if (query.hasQueryItem("pbk"))
        {
            const auto pbk = QUrl::fromPercentEncoding(query.queryItemValue("pbk").toUtf8());
            QJsonIO::SetValue(stream, pbk, { "realitySettings", "publicKey" });
        }
        if (query.hasQueryItem("spiderX"))
        {
            const auto spiderX = QUrl::fromPercentEncoding(query.queryItemValue("spiderX").toUtf8());
            QJsonIO::SetValue(stream, spiderX, { "realitySettings", "spiderX" });
        }
        if (query.hasQueryItem("sid"))
        {
            const auto sid = QUrl::fromPercentEncoding(query.queryItemValue("sid").toUtf8());
            QJsonIO::SetValue(stream, sid, { "realitySettings", "shortId" });
        }
    }

    QJsonObject root;
    QJsonArray outbounds;
    QJsonObject outbound = outbounds::GenerateOutboundEntry(OUTBOUND_TAG_PROXY, "trojan", outbounds::GenerateTrojanOUT({ result }), {});
    outbound["streamSettings"] = stream;
    outbounds.append(outbound);
    JADD(outbounds)
    QJsonObject inbound = inbounds::GenerateInboundEntry();
    root["inbounds"] = QJsonArray { inbound };

    return root;
}

} // namespace amnezia::serialization::trojan

