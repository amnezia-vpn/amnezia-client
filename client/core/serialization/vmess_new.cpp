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
#include "3rd/QJsonStruct/QJsonStruct.hpp"
#include "transfer.h"
#include "serialization.h"

#include <QUrlQuery>

#define OUTBOUND_TAG_PROXY "PROXY"

namespace amnezia::serialization::vmess_new
{
const static QStringList NetworkType{ "tcp", "http", "ws", "kcp", "quic", "grpc" };
const static QStringList QuicSecurityTypes{ "none", "aes-128-gcm", "chacha20-poly1305" };
const static QStringList QuicKcpHeaderTypes{ "none", "srtp", "utp", "wechat-video", "dtls", "wireguard" };
const static QStringList FalseTypes{ "false", "False", "No", "Off", "0" };

QJsonObject Deserialize(const QString &vmessStr, QString *alias, QString *errMessage)
{
    QUrl url{ vmessStr };
    QUrlQuery query{ url };
    //
#define default QJsonObject()
    if (!url.isValid())
    {
        *errMessage = QObject::tr("vmess:// url is invalid");
        return default;
    }

    // If previous alias is empty, just the PS is needed, else, append a "_"
    const auto name = url.fragment(QUrl::FullyDecoded).trimmed();
    *alias = alias->isEmpty() ? name : (*alias + "_" + name);

    VMessServerObject server;
    server.users << VMessServerObject::UserObject{};

    StreamSettingsObject stream;
    QString net;
    bool tls = false;
    // Check streamSettings
    {
        for (const auto &_protocol : url.userName().split("+"))
        {
            if (_protocol == "tls")
                tls = true;
            else
                net = _protocol;
        }
        if (!NetworkType.contains(net))
        {
            *errMessage = QObject::tr("Invalid streamSettings protocol: ") + net;
            return default;
        }
        stream.network = net;
        stream.security = tls ? "tls" : "";
    }
    // Host Port UUID AlterID
    {
        const auto host = url.host();
        int port = url.port();
        QString uuid;
        int aid;
        {
            const auto pswd = url.password();
            const auto index = pswd.lastIndexOf("-");
            uuid = pswd.mid(0, index);
            aid = pswd.right(pswd.length() - index - 1).toInt();
        }
        server.address = host;
        server.port = port;
        server.users.first().id = uuid;
        server.users.first().alterId = aid;
        server.users.first().security = "auto";
    }

    const auto getQueryValue = [&query](const QString &key, const QString &defaultValue) {
        if (query.hasQueryItem(key))
            return query.queryItemValue(key, QUrl::FullyDecoded);
        else
            return defaultValue;
    };

    //
    // Begin transport settings parser
    {
        if (net == "tcp")
        {
            stream.tcpSettings.header.type = getQueryValue("type", "none");
        }
        else if (net == "http")
        {
            stream.httpSettings.host.append(getQueryValue("host", ""));
            stream.httpSettings.path = getQueryValue("path", "/");
        }
        else if (net == "ws")
        {
            stream.wsSettings.headers["Host"] = getQueryValue("host", "");
            stream.wsSettings.path = getQueryValue("path", "/");
        }
        else if (net == "kcp")
        {
            stream.kcpSettings.seed = getQueryValue("seed", "");
            stream.kcpSettings.header.type = getQueryValue("type", "none");
        }
        else if (net == "quic")
        {
            stream.quicSettings.security = getQueryValue("security", "none");
            stream.quicSettings.key = getQueryValue("key", "");
            stream.quicSettings.header.type = getQueryValue("type", "none");
        }
        else if (net == "grpc")
        {
            stream.grpcSettings.serviceName = getQueryValue("serviceName", "");
        }
        else
        {
            *errMessage = QObject::tr("Unknown transport method: ") + net;
            return default;
        }
    }
#undef default
    if (tls)
    {
        stream.tlsSettings.allowInsecure = !FalseTypes.contains(getQueryValue("allowInsecure", "false"));
        stream.tlsSettings.serverName = getQueryValue("tlsServerName", "");
    }
    QJsonObject root;
    QJsonObject vConf;
    QJsonArray vnextArray;
    vnextArray.append(server.toJson());
    vConf["vnext"] = vnextArray;
    auto outbound = outbounds::GenerateOutboundEntry(OUTBOUND_TAG_PROXY, "vmess", vConf, stream.toJson());
    QJsonObject inbound = inbounds::GenerateInboundEntry();

    //
    root["outbounds"] = QJsonArray{ outbound };
    root["inbound"] = QJsonArray{ inbound };
    return root;
}

} // namespace amnezia::serialization::vmess_new
