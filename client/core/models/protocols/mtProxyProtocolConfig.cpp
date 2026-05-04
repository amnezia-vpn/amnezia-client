#include "mtProxyProtocolConfig.h"

#include "../../../core/utils/protocolEnum.h"
#include "../../../core/protocols/protocolUtils.h"
#include "../../../core/utils/constants/configKeys.h"
#include "../../../core/utils/constants/protocolConstants.h"
#include <QJsonArray>

#include <algorithm>

using namespace amnezia;

namespace amnezia {

    QJsonObject MtProxyProtocolConfig::toJson() const {
        QJsonObject obj;

        if (!port.isEmpty()) {
            obj[configKey::port] = port;
        }
        if (!secret.isEmpty()) {
            obj[protocols::mtProxy::secretKey] = secret;
        }
        if (!tag.isEmpty()) {
            obj[protocols::mtProxy::tagKey] = tag;
        }
        if (!tgLink.isEmpty()) {
            obj[protocols::mtProxy::tgLinkKey] = tgLink;
        }
        if (!tmeLink.isEmpty()) {
            obj[protocols::mtProxy::tmeLinkKey] = tmeLink;
        }
        obj[protocols::mtProxy::isEnabledKey] = isEnabled;
        if (!publicHost.isEmpty()) {
            obj[protocols::mtProxy::publicHostKey] = publicHost;
        }
        if (!transportMode.isEmpty()) {
            obj[protocols::mtProxy::transportModeKey] = transportMode;
        }
        if (!tlsDomain.isEmpty()) {
            obj[protocols::mtProxy::tlsDomainKey] = tlsDomain;
        }
        if (!additionalSecrets.isEmpty()) {
            obj[protocols::mtProxy::additionalSecretsKey] = QJsonArray::fromStringList(additionalSecrets);
        }
        if (!workersMode.isEmpty()) {
            obj[protocols::mtProxy::workersModeKey] = workersMode;
        }
        if (!workers.isEmpty()) {
            obj[protocols::mtProxy::workersKey] = workers;
        }
        obj[protocols::mtProxy::natEnabledKey] = natEnabled;
        if (!natInternalIp.isEmpty()) {
            obj[protocols::mtProxy::natInternalIpKey] = natInternalIp;
        }
        if (!natExternalIp.isEmpty()) {
            obj[protocols::mtProxy::natExternalIpKey] = natExternalIp;
        }

        return obj;
    }

    MtProxyProtocolConfig MtProxyProtocolConfig::fromJson(const QJsonObject &json) {
        MtProxyProtocolConfig config;

        config.port = json.value(configKey::port).toString();
        config.secret = json.value(protocols::mtProxy::secretKey).toString();
        config.tag = json.value(protocols::mtProxy::tagKey).toString();
        config.tgLink = json.value(protocols::mtProxy::tgLinkKey).toString();
        config.tmeLink = json.value(protocols::mtProxy::tmeLinkKey).toString();
        config.isEnabled = json.value(protocols::mtProxy::isEnabledKey).toBool(true);
        config.publicHost = json.value(protocols::mtProxy::publicHostKey).toString();
        config.transportMode = json.value(protocols::mtProxy::transportModeKey).toString();
        config.tlsDomain = json.value(protocols::mtProxy::tlsDomainKey).toString();
        for (const auto &v: json.value(protocols::mtProxy::additionalSecretsKey).toArray()) {
            const QString s = v.toString();
            if (!s.isEmpty()) {
                config.additionalSecrets.append(s);
            }
        }
        config.workersMode = json.value(protocols::mtProxy::workersModeKey).toString();
        config.workers = json.value(protocols::mtProxy::workersKey).toString();
        config.natEnabled = json.value(protocols::mtProxy::natEnabledKey).toBool(false);
        config.natInternalIp = json.value(protocols::mtProxy::natInternalIpKey).toString();
        config.natExternalIp = json.value(protocols::mtProxy::natExternalIpKey).toString();

        return config;
    }

    bool MtProxyProtocolConfig::equalsDockerDeploymentSettings(const MtProxyProtocolConfig &other) const {
        const auto normPort = [](const QString &p) {
            return p.isEmpty() ? QString(protocols::mtProxy::defaultPort) : p;
        };
        const auto normTransport = [](const QString &t) {
            return t.isEmpty() ? QString(protocols::mtProxy::transportModeStandard) : t;
        };
        const auto normWorkersMode = [](const QString &m) {
            return m.isEmpty() ? QString(protocols::mtProxy::workersModeAuto) : m;
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

} // namespace amnezia
