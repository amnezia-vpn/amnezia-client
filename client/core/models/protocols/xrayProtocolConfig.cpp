#include "xrayProtocolConfig.h"

#include <QJsonArray>
#include <QJsonDocument>
#include "protocols/protocols_defs.h"

using namespace amnezia;

XrayProtocolConfig::XrayProtocolConfig(const QJsonObject &protocolConfigObject, const QString &protocolName) : ProtocolConfig(protocolName)
{
    serverProtocolConfig.site = protocolConfigObject.value(config_key::site).toString();
    serverProtocolConfig.port = protocolConfigObject.value(config_key::port).toString();
    serverProtocolConfig.transportProto = protocolConfigObject.value(config_key::transport_proto).toString();

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
