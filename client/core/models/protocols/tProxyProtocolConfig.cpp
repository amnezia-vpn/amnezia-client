#include "tProxyProtocolConfig.h"

#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"

using namespace amnezia;

QJsonObject TProxyProtocolConfig::toJson() const
{
    QJsonObject obj;
    if (!port.isEmpty()) {
        obj[QString(configKey::port)] = port;
    }
    if (!httpPort.isEmpty()) {
        obj[protocols::tProxy::httpPortKey] = httpPort;
    }
    if (!secret.isEmpty()) {
        obj[protocols::tProxy::secretKey] = secret;
    }
    if (!hostname.isEmpty()) {
        obj[protocols::tProxy::hostnameKey] = hostname;
    }
    if (!acmeEmail.isEmpty()) {
        obj[protocols::tProxy::acmeEmailKey] = acmeEmail;
    }
    if (!carrierMode.isEmpty()) {
        obj[protocols::tProxy::carrierModeKey] = carrierMode;
    }
    if (!workers.isEmpty()) {
        obj[protocols::tProxy::workersKey] = workers;
    }
    if (!tgLink.isEmpty()) {
        obj[protocols::tProxy::tgLinkKey] = tgLink;
    }
    if (!tmeLink.isEmpty()) {
        obj[protocols::tProxy::tmeLinkKey] = tmeLink;
    }
    obj[protocols::tProxy::isEnabledKey] = isEnabled;
    return obj;
}

TProxyProtocolConfig TProxyProtocolConfig::fromJson(const QJsonObject &json)
{
    TProxyProtocolConfig c;
    c.port = json.value(QString(configKey::port)).toString();
    c.httpPort = json.value(protocols::tProxy::httpPortKey).toString();
    c.secret = json.value(protocols::tProxy::secretKey).toString();
    c.hostname = json.value(protocols::tProxy::hostnameKey).toString();
    c.acmeEmail = json.value(protocols::tProxy::acmeEmailKey).toString();
    c.carrierMode = json.value(protocols::tProxy::carrierModeKey).toString();
    c.workers = json.value(protocols::tProxy::workersKey).toString();
    c.tgLink = json.value(protocols::tProxy::tgLinkKey).toString();
    c.tmeLink = json.value(protocols::tProxy::tmeLinkKey).toString();
    c.isEnabled = json.value(protocols::tProxy::isEnabledKey).toBool(true);
    return c;
}

bool TProxyProtocolConfig::equalsDockerDeploymentSettings(const TProxyProtocolConfig &other) const
{
    const auto normCarrier = [](const QString &m) {
        return m.isEmpty() ? QString(protocols::tProxy::carrierModeHttps) : m;
    };
    const auto normWorkers = [](const QString &w) {
        return w.isEmpty() ? QString(protocols::tProxy::defaultWorkers) : w;
    };

    const auto normPort = [](const QString &p, const char *defaultPort) {
        return p.isEmpty() ? QString(defaultPort) : p;
    };

    return hostname == other.hostname && secret == other.secret && acmeEmail == other.acmeEmail
            && normPort(port, protocols::tProxy::defaultPort) == normPort(other.port, protocols::tProxy::defaultPort)
            && normPort(httpPort, protocols::tProxy::defaultHttpPort)
                    == normPort(other.httpPort, protocols::tProxy::defaultHttpPort)
            && normCarrier(carrierMode) == normCarrier(other.carrierMode)
            && normWorkers(workers) == normWorkers(other.workers) && isEnabled == other.isEnabled;
}
