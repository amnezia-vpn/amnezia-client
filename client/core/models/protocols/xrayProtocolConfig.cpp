#include "xrayProtocolConfig.h"

#include <QJsonArray>
#include <QJsonDocument>
#include "protocols/protocols_defs.h"

using namespace amnezia;

XrayProtocolConfig::XrayProtocolConfig(const QJsonObject &protocolConfigObject, const QString &protocolName) : ProtocolConfig(protocolName)
{
    serverProtocolConfig.site = protocolConfigObject.value(config_key::site).toString(protocols::xray::defaultSite);
    serverProtocolConfig.port = protocolConfigObject.value(config_key::port).toString(protocols::xray::defaultPort);
    serverProtocolConfig.transportProto = protocolConfigObject.value(config_key::transport_proto).toString("tcp");

    auto clientProtocolString = protocolConfigObject.value(config_key::last_config).toString();
    if (!clientProtocolString.isEmpty()) {
        clientProtocolConfig.isEmpty = false;

        QJsonObject clientProtocolConfigObject = QJsonDocument::fromJson(clientProtocolString.toUtf8()).object();
    }
}

QJsonObject XrayProtocolConfig::toJson() const
{
    QJsonObject json;

    if (!serverProtocolConfig.site.isEmpty()) {
        json[config_key::site] = serverProtocolConfig.site;
    }
    if (!serverProtocolConfig.port.isEmpty()) {
        json[config_key::port] = serverProtocolConfig.port;
    }
    if (!serverProtocolConfig.transportProto.isEmpty()) {
        json[config_key::transport_proto] = serverProtocolConfig.transportProto;
    }

    if (!clientProtocolConfig.isEmpty) {
        QJsonObject clientConfigJson;
        json[config_key::last_config] = QString(QJsonDocument(clientConfigJson).toJson());
    }

    return json;
}

bool XrayProtocolConfig::hasEqualServerSettings(const XrayProtocolConfig &other) const
{
    if (serverProtocolConfig.site != other.serverProtocolConfig.site || 
        serverProtocolConfig.port != other.serverProtocolConfig.port ||
        serverProtocolConfig.transportProto != other.serverProtocolConfig.transportProto) {
        return false;
    }
    return true;
}

void XrayProtocolConfig::clearClientSettings()
{
    clientProtocolConfig = xray::ClientProtocolConfig();
} 
