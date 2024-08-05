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
#include "utilities.h"
#include "serialization.h"

#define OUTBOUND_TAG_PROXY "PROXY"
#define JADD(...) FOR_EACH(JADDEx, __VA_ARGS__)

namespace amnezia::serialization::ss
{
QJsonObject Deserialize(const QString &ssUri, QString *alias, QString *errMessage)
{
    ShadowSocksServerObject server;
    QString d_name;

    // auto ssUri = _ssUri.toStdString();
    if (ssUri.length() < 5)
    {
        *errMessage = QObject::tr("SS URI is too short");
    }

    auto uri = ssUri.mid(5);
    auto hashPos = uri.lastIndexOf("#");

    if (hashPos >= 0)
    {
        // Get the name/remark
        d_name = uri.mid(uri.lastIndexOf("#") + 1);
        uri.truncate(hashPos);
    }

    auto atPos = uri.indexOf('@');

    if (atPos < 0)
    {
        // Old URI scheme
        QString decoded = QByteArray::fromBase64(uri.toUtf8(), QByteArray::Base64Option::OmitTrailingEquals);
        auto colonPos = decoded.indexOf(':');

        if (colonPos < 0)
        {
            *errMessage = QObject::tr("Can't find the colon separator between method and password");
        }

        server.method = decoded.left(colonPos);
        decoded.remove(0, colonPos + 1);
        atPos = decoded.lastIndexOf('@');

        if (atPos < 0)
        {
            *errMessage = QObject::tr("Can't find the at separator between password and hostname");
        }

        server.password = decoded.mid(0, atPos);
        decoded.remove(0, atPos + 1);
        colonPos = decoded.lastIndexOf(':');

        if (colonPos < 0)
        {
            *errMessage = QObject::tr("Can't find the colon separator between hostname and port");
        }

        server.address = decoded.mid(0, colonPos);
        server.port = decoded.mid(colonPos + 1).toInt();
    }
    else
    {
        // SIP002 URI scheme
        auto x = QUrl::fromUserInput(uri);
        server.address = x.host();
        server.port = x.port();
        const auto userInfo = Utils::SafeBase64Decode(x.userName());
        const auto userInfoSp = userInfo.indexOf(':');

        if (userInfoSp < 0)
        {
            *errMessage = QObject::tr("Can't find the colon separator between method and password");
            return QJsonObject{};
        }

        const auto method = userInfo.mid(0, userInfoSp);
        server.method = method;
        server.password = userInfo.mid(userInfoSp + 1);
    }

    d_name = QUrl::fromPercentEncoding(d_name.toUtf8());
    QJsonObject root;
    QJsonArray outbounds;
    outbounds.append(outbounds::GenerateOutboundEntry(OUTBOUND_TAG_PROXY, "shadowsocks", outbounds::GenerateShadowSocksOUT({ server }), {}));
    JADD(outbounds)
    QJsonObject inbound = inbounds::GenerateInboundEntry();
    root["inbounds"] = QJsonArray{ inbound };
    *alias = alias->isEmpty() ? d_name : *alias + "_" + d_name;
    return root;
}

const QString Serialize(const ShadowSocksServerObject &server, const QString &alias, bool)
{
    QUrl url;
    const auto plainUserInfo = server.method + ":" + server.password;
    const auto userinfo = plainUserInfo.toUtf8().toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    url.setUserInfo(userinfo);
    url.setScheme("ss");
    url.setHost(server.address);
    url.setPort(server.port);
    url.setFragment(alias);
    return url.toString(QUrl::ComponentFormattingOption::FullyEncoded);
}
} // namespace amnezia::serialization::ss

