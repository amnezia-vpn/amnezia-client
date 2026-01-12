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

#include "3rd/QJsonStruct/QJsonStruct.hpp"
#include <QJsonDocument>
#include "transfer.h"
#include "utilities.h"
#include "serialization.h"

#define nothing

#define OUTBOUND_TAG_PROXY "PROXY"

namespace amnezia::serialization::vmess
{
// From https://github.com/2dust/v2rayN/wiki/分享链接格式说明(ver-2)
const QString Serialize(const StreamSettingsObject &transfer, const VMessServerObject &server, const QString &alias)
{
    QJsonObject vmessUriRoot;
    // Constant
    vmessUriRoot["v"] = 2;
    vmessUriRoot["ps"] = alias;
    vmessUriRoot["add"] = server.address;
    vmessUriRoot["port"] = server.port;
    vmessUriRoot["id"] = server.users.front().id;
    vmessUriRoot["aid"] = server.users.front().alterId;
    const auto scy = server.users.front().security;
    vmessUriRoot["scy"] = (scy == "aes-128-gcm" || scy == "chacha20-poly1305" || scy == "none" || scy == "zero") ? scy : "auto";
    vmessUriRoot["net"] = transfer.network == "http" ? "h2" : transfer.network;
    vmessUriRoot["tls"] = (transfer.security == "tls" || transfer.security == "xtls") ? "tls" : "none";
    if (transfer.security == "tls")
    {
        vmessUriRoot["sni"] = transfer.tlsSettings.serverName;
    }
    else if (transfer.security == "xtls")
    {
        vmessUriRoot["sni"] = transfer.xtlsSettings.serverName;
    }

    if (transfer.network == "tcp")
    {
        vmessUriRoot["type"] = transfer.tcpSettings.header.type;
    }
    else if (transfer.network == "kcp")
    {
        vmessUriRoot["type"] = transfer.kcpSettings.header.type;
    }
    else if (transfer.network == "quic")
    {
        vmessUriRoot["type"] = transfer.quicSettings.header.type;
        vmessUriRoot["host"] = transfer.quicSettings.security;
        vmessUriRoot["path"] = transfer.quicSettings.key;
    }
    else if (transfer.network == "ws")
    {
        auto x = transfer.wsSettings.headers;
        auto host = x.contains("host");
        auto CapHost = x.contains("Host");
        auto realHost = host ? x["host"] : (CapHost ? x["Host"] : "");
        //
        vmessUriRoot["host"] = realHost;
        vmessUriRoot["path"] = transfer.wsSettings.path;
    }
    else if (transfer.network == "h2" || transfer.network == "http")
    {
        vmessUriRoot["host"] = transfer.httpSettings.host.join(",");
        vmessUriRoot["path"] = transfer.httpSettings.path;
    }
    else if (transfer.network == "grpc")
    {
        vmessUriRoot["path"] = transfer.grpcSettings.serviceName;
    }

    if (!vmessUriRoot.contains("type") || vmessUriRoot["type"].toString().isEmpty())
    {
        vmessUriRoot["type"] = "none";
    }

    //
    QString jString = Utils::JsonToString(vmessUriRoot, QJsonDocument::JsonFormat::Compact);
    auto vmessPart = jString.toUtf8().toBase64();
    return "vmess://" + vmessPart;
}

// This generates global config containing only one outbound....
QJsonObject Deserialize(const QString &vmessStr, QString *alias, QString *errMessage)
{
#define default QJsonObject()
    QString vmess = vmessStr;

    if (vmess.trimmed() != vmess)
    {
        vmess = vmessStr.trimmed();
    }

    // Reset errMessage
    *errMessage = "";

    if (!vmess.toLower().startsWith("vmess://"))
    {
        *errMessage = QObject::tr("VMess string should start with 'vmess://'");
        return default;
    }

    const auto b64Str = vmess.mid(8, vmess.length() - 8);
    if (b64Str.isEmpty())
    {
        *errMessage = QObject::tr("VMess string should be a valid base64 string");
        return default;
    }

    auto vmessString = Utils::SafeBase64Decode(b64Str);
    auto jsonErr = Utils::VerifyJsonString(vmessString);

    if (!jsonErr.isEmpty())
    {
        *errMessage = jsonErr;
        return default;
    }

    auto vmessConf = Utils::JsonFromString(vmessString);

    if (vmessConf.isEmpty())
    {
        *errMessage = QObject::tr("JSON should not be empty");
        return default;
    }

    // --------------------------------------------------------------------------------------
    QJsonObject root;
    QString ps, add, id, net, type, host, path, tls, scy, sni;
    int port, aid;
    //
    // __vmess_checker__func(key, values)
    //
    //   - Key     =    Key in JSON and the variable name.
    //   - Values  =    Candidate variable list, if not match, the first one is used as default.
    //
    //   - [[val.size() <= 1]] is used when only the default value exists.
    //
    //   - It can be empty, if so,           if the key is not in the JSON, or the value is empty,  report an error.
    //   - Else if it contains one thing.    if the key is not in the JSON, or the value is empty,  use that one.
    //   - Else if it contains many things,  when the key IS in the JSON but not within the THINGS, use the first in the THINGS
    //   - Else -------------------------------------------->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>  use the JSON value
    //
#define __vmess_checker__func(key, values)                                                                                                               \
    {                                                                                                                                                    \
            auto val = QStringList() values;                                                                                                             \
            if (vmessConf.contains(#key) && !vmessConf[#key].toVariant().toString().trimmed().isEmpty() &&                                               \
                (val.size() <= 1 || val.contains(vmessConf[#key].toVariant().toString())))                                                               \
        {                                                                                                                                                \
                key = vmessConf[#key].toVariant().toString();                                                                                            \
        }                                                                                                                                                \
            else if (!val.isEmpty())                                                                                                                     \
        {                                                                                                                                                \
                key = val.first();                                                                                                                       \
        }                                                                                                                                                \
            else                                                                                                                                         \
        {                                                                                                                                                \
                *errMessage = QObject::tr(#key " does not exist.");                                                                                      \
        }                                                                                                                                                \
    }

    // vmess v1 upgrader
    if (!vmessConf.contains("v"))
    {
        qDebug() << "Detected deprecated vmess v1. Trying to upgrade...";
        if (const auto network = vmessConf["net"].toString(); network == "ws" || network == "h2")
        {
            const QStringList hostComponents = vmessConf["host"].toString().replace(" ", "").split(";");
            if (const auto nParts = hostComponents.length(); nParts == 1)
                vmessConf["path"] = hostComponents[0], vmessConf["host"] = "";
            else if (nParts == 2)
                vmessConf["path"] = hostComponents[0], vmessConf["host"] = hostComponents[1];
            else
                vmessConf["path"] = "/", vmessConf["host"] = "";
        }
    }

    // Strict check of VMess protocol, to check if the specified value
    // is in the correct range.
    //
    // Get Alias (AKA ps) from address and port.
    {
        // Some idiot vmess:// links are using alterId...
        aid = vmessConf.contains("aid") ? vmessConf.value("aid").toInt(VMESS_USER_ALTERID_DEFAULT) :
                  vmessConf.value("alterId").toInt(VMESS_USER_ALTERID_DEFAULT);
        //
        //
        __vmess_checker__func(ps, << vmessConf["add"].toVariant().toString() + ":" + vmessConf["port"].toVariant().toString()); //
        __vmess_checker__func(add, nothing);                                                                                    //
        __vmess_checker__func(id, nothing);                                                                                     //
        __vmess_checker__func(scy, << "aes-128-gcm"                                                                             //
                                   << "chacha20-poly1305"                                                                       //
                                   << "auto"                                                                                    //
                                   << "none"                                                                                    //
                                   << "zero");                                                                                  //
        //
        __vmess_checker__func(type, << "none"                                                                                   //
                                    << "http"                                                                                   //
                                    << "srtp"                                                                                   //
                                    << "utp"                                                                                    //
                                    << "wechat-video");                                                                         //
        //
        __vmess_checker__func(net, << "tcp"                                                                                     //
                                   << "http"                                                                                    //
                                   << "h2"                                                                                      //
                                   << "ws"                                                                                      //
                                   << "kcp"                                                                                     //
                                   << "quic"                                                                                    //
                                   << "grpc");                                                                                  //
        //
        __vmess_checker__func(tls, << "none"                                                                                    //
                                   << "tls");                                                                                   //
        //
        path = vmessConf.contains("path") ? vmessConf["path"].toVariant().toString() : (net == "quic" ? "" : "/");
        host = vmessConf.contains("host") ? vmessConf["host"].toVariant().toString() : (net == "quic" ? "none" : "");
    }

    // Respect connection type rather than obfs type
    if (QStringList{ "srtp", "utp", "wechat-video" }.contains(type)) //
    {                                                                //
        if (net != "quic" && net != "kcp")                           //
        {                                                            //
            type = "none";                                           //
        }                                                            //
    }

    port = vmessConf["port"].toVariant().toInt();
    aid = vmessConf["aid"].toVariant().toInt();
    //
    // Apply the settings.
    // User
    VMessServerObject::UserObject user;
    user.id = id;
    user.alterId = aid;
    user.security = scy;
    //
    // Server
    VMessServerObject serv;
    serv.port = port;
    serv.address = add;
    serv.users.push_back(user);
    //
    //
    // Stream Settings
    StreamSettingsObject streaming;

    if (net == "tcp")
    {
        streaming.tcpSettings.header.type = type;
    }
    else if (net == "http" || net == "h2")
    {
        // Fill hosts for HTTP
        for (const auto &_host : host.split(','))
        {
            if (!_host.isEmpty())
            {
                streaming.httpSettings.host << _host.trimmed();
            }
        }

        streaming.httpSettings.path = path;
    }
    else if (net == "ws")
    {
        if (!host.isEmpty())
            streaming.wsSettings.headers["Host"] = host;
        streaming.wsSettings.path = path;
    }
    else if (net == "kcp")
    {
        streaming.kcpSettings.header.type = type;
    }
    else if (net == "quic")
    {
        streaming.quicSettings.security = host;
        streaming.quicSettings.header.type = type;
        streaming.quicSettings.key = path;
    }
    else if (net == "grpc")
    {
        streaming.grpcSettings.serviceName = path;
    }

    streaming.security = tls;
    if (tls == "tls")
    {
        if (sni.isEmpty() && !host.isEmpty())
            sni = host;
        streaming.tlsSettings.serverName = sni;
        streaming.tlsSettings.allowInsecure = false;
    }
    //
    // Network type
    // NOTE(DuckSoft): Damn vmess:// just don't write 'http' properly
    if (net == "h2")
        net = "http";
    streaming.network = net;
    //
    // VMess root config
    QJsonObject vConf;
    vConf["vnext"] = QJsonArray{ serv.toJson() };
    const auto outbound = outbounds::GenerateOutboundEntry(OUTBOUND_TAG_PROXY, "vmess", vConf, streaming.toJson());
    QJsonObject inbound = inbounds::GenerateInboundEntry();
    root["outbounds"] = QJsonArray{ outbound };
    root["inbounds"] = QJsonArray{ inbound };
    // If previous alias is empty, just the PS is needed, else, append a "_"
    *alias = alias->trimmed().isEmpty() ? ps : *alias + "_" + ps;
    return root;
#undef default
}
} // namespace amnezia::serialization::vmess

