#ifndef TELEMTPROTOCOLCONFIG_H
#define TELEMTPROTOCOLCONFIG_H

#include <QJsonObject>
#include <QString>
#include <QStringList>

namespace amnezia {

struct TelemtProtocolConfig {
    QString port;
    QString secret;
    QString tag;
    QString tgLink;
    QString tmeLink;
    bool isEnabled = true;
    QString publicHost;
    QString transportMode;
    QString tlsDomain;
    bool maskEnabled = true;
    bool tlsEmulation = false;
    bool useMiddleProxy = true;
    QString userName;
    QStringList additionalSecrets;
    QString workersMode;
    QString workers;
    bool natEnabled = false;
    QString natInternalIp;
    QString natExternalIp;

    QJsonObject toJson() const;
    static TelemtProtocolConfig fromJson(const QJsonObject &json);
    bool equalsDockerDeploymentSettings(const TelemtProtocolConfig &other) const;
};

} // namespace amnezia

#endif // TELEMTPROTOCOLCONFIG_H
