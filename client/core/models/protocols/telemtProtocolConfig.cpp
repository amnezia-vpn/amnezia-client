#include "telemtProtocolConfig.h"

#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"

#include <QJsonArray>
#include <algorithm>

using namespace amnezia;

QJsonObject TelemtProtocolConfig::toJson() const
{
    QJsonObject obj;
    if (!port.isEmpty()) {
        obj[QString(configKey::port)] = port;
    }
    if (!secret.isEmpty()) {
        obj[protocols::telemt::secretKey] = secret;
    }
    if (!tag.isEmpty()) {
        obj[protocols::telemt::tagKey] = tag;
    }
    if (!tgLink.isEmpty()) {
        obj[protocols::telemt::tgLinkKey] = tgLink;
    }
    if (!tmeLink.isEmpty()) {
        obj[protocols::telemt::tmeLinkKey] = tmeLink;
    }
    obj[protocols::telemt::isEnabledKey] = isEnabled;
    if (!publicHost.isEmpty()) {
        obj[protocols::telemt::publicHostKey] = publicHost;
    }
    if (!transportMode.isEmpty()) {
        obj[protocols::telemt::transportModeKey] = transportMode;
    }
    if (!tlsDomain.isEmpty()) {
        obj[protocols::telemt::tlsDomainKey] = tlsDomain;
    }
    obj[protocols::telemt::maskEnabledKey] = maskEnabled;
    obj[protocols::telemt::tlsEmulationKey] = tlsEmulation;
    obj[protocols::telemt::useMiddleProxyKey] = useMiddleProxy;
    if (!userName.isEmpty()) {
        obj[protocols::telemt::userNameKey] = userName;
    }
    if (!additionalSecrets.isEmpty()) {
        obj[protocols::telemt::additionalSecretsKey] = QJsonArray::fromStringList(additionalSecrets);
    }
    if (!workersMode.isEmpty()) {
        obj[protocols::telemt::workersModeKey] = workersMode;
    }
    if (!workers.isEmpty()) {
        obj[protocols::telemt::workersKey] = workers;
    }
    obj[protocols::telemt::natEnabledKey] = natEnabled;
    if (!natInternalIp.isEmpty()) {
        obj[protocols::telemt::natInternalIpKey] = natInternalIp;
    }
    if (!natExternalIp.isEmpty()) {
        obj[protocols::telemt::natExternalIpKey] = natExternalIp;
    }
    return obj;
}

TelemtProtocolConfig TelemtProtocolConfig::fromJson(const QJsonObject &json)
{
    TelemtProtocolConfig c;
    c.port = json.value(QString(configKey::port)).toString();
    c.secret = json.value(protocols::telemt::secretKey).toString();
    c.tag = json.value(protocols::telemt::tagKey).toString();
    c.tgLink = json.value(protocols::telemt::tgLinkKey).toString();
    c.tmeLink = json.value(protocols::telemt::tmeLinkKey).toString();
    c.isEnabled = json.value(protocols::telemt::isEnabledKey).toBool(true);
    c.publicHost = json.value(protocols::telemt::publicHostKey).toString();
    c.transportMode = json.value(protocols::telemt::transportModeKey).toString();
    c.tlsDomain = json.value(protocols::telemt::tlsDomainKey).toString();
    c.maskEnabled = json.value(protocols::telemt::maskEnabledKey).toBool(true);
    c.tlsEmulation = json.value(protocols::telemt::tlsEmulationKey).toBool(false);
    c.useMiddleProxy = json.value(protocols::telemt::useMiddleProxyKey).toBool(true);
    c.userName = json.value(protocols::telemt::userNameKey).toString();
    for (const auto &v : json.value(protocols::telemt::additionalSecretsKey).toArray()) {
        const QString s = v.toString();
        if (!s.isEmpty()) {
            c.additionalSecrets.append(s);
        }
    }
    c.workersMode = json.value(protocols::telemt::workersModeKey).toString();
    c.workers = json.value(protocols::telemt::workersKey).toString();
    c.natEnabled = json.value(protocols::telemt::natEnabledKey).toBool(false);
    c.natInternalIp = json.value(protocols::telemt::natInternalIpKey).toString();
    c.natExternalIp = json.value(protocols::telemt::natExternalIpKey).toString();
    return c;
}

bool TelemtProtocolConfig::equalsDockerDeploymentSettings(const TelemtProtocolConfig &other) const
{
    const auto normPort = [](const QString &p) {
        return p.isEmpty() ? QString(protocols::telemt::defaultPort) : p;
    };
    const auto normTransport = [](const QString &t) {
        return t.isEmpty() ? QString(protocols::telemt::transportModeStandard) : t;
    };
    const auto normWorkersMode = [](const QString &m) {
        return m.isEmpty() ? QString(protocols::telemt::workersModeAuto) : m;
    };

    if (normPort(port) != normPort(other.port)) {
        return false;
    }
    if (normTransport(transportMode) != normTransport(other.transportMode)) {
        return false;
    }
    if (tlsDomain != other.tlsDomain) {
        return false;
    }
    if (secret != other.secret) {
        return false;
    }
    if (tag != other.tag) {
        return false;
    }
    if (publicHost != other.publicHost) {
        return false;
    }
    if (maskEnabled != other.maskEnabled) {
        return false;
    }
    if (tlsEmulation != other.tlsEmulation) {
        return false;
    }
    if (useMiddleProxy != other.useMiddleProxy) {
        return false;
    }
    if (userName != other.userName) {
        return false;
    }
    if (normWorkersMode(workersMode) != normWorkersMode(other.workersMode)) {
        return false;
    }
    if (workers != other.workers) {
        return false;
    }
    if (natEnabled != other.natEnabled) {
        return false;
    }
    if (natInternalIp != other.natInternalIp) {
        return false;
    }
    if (natExternalIp != other.natExternalIp) {
        return false;
    }
    if (isEnabled != other.isEnabled) {
        return false;
    }

    QStringList aa = additionalSecrets;
    QStringList bb = other.additionalSecrets;
    aa.removeAll(QString());
    bb.removeAll(QString());
    std::sort(aa.begin(), aa.end());
    std::sort(bb.begin(), bb.end());
    return aa == bb;
}
