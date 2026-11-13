#include "whatsAppProxyProtocolConfig.h"

#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"

using namespace amnezia;

QString WhatsAppProxyProtocolConfig::effectivePort() const
{
    return port.isEmpty() ? QString(protocols::whatsAppProxy::defaultPort) : port;
}

QString WhatsAppProxyProtocolConfig::effectiveMediaPort() const
{
    return mediaPort.isEmpty() ? QString(protocols::whatsAppProxy::defaultMediaPort) : mediaPort;
}

QJsonObject WhatsAppProxyProtocolConfig::toJson() const
{
    QJsonObject obj;
    if (!port.isEmpty()) {
        obj[QString(configKey::port)] = port;
    }
    if (!mediaPort.isEmpty()) {
        obj[protocols::whatsAppProxy::mediaPortKey] = mediaPort;
    }
    if (!publicHost.isEmpty()) {
        obj[protocols::whatsAppProxy::publicHostKey] = publicHost;
    }
    obj[protocols::whatsAppProxy::isEnabledKey] = isEnabled;
    return obj;
}

WhatsAppProxyProtocolConfig WhatsAppProxyProtocolConfig::fromJson(const QJsonObject &json)
{
    WhatsAppProxyProtocolConfig c;
    c.port = json.value(QString(configKey::port)).toString();
    c.mediaPort = json.value(protocols::whatsAppProxy::mediaPortKey).toString();
    c.publicHost = json.value(protocols::whatsAppProxy::publicHostKey).toString();
    c.isEnabled = json.value(protocols::whatsAppProxy::isEnabledKey).toBool(true);
    return c;
}

bool WhatsAppProxyProtocolConfig::equalsDockerDeploymentSettings(const WhatsAppProxyProtocolConfig &other) const
{
    return effectivePort() == other.effectivePort() && effectiveMediaPort() == other.effectiveMediaPort();
}
