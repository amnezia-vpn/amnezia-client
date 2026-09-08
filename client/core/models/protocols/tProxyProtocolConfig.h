#ifndef TPROXYPROTOCOLCONFIG_H
#define TPROXYPROTOCOLCONFIG_H

#include <QJsonObject>
#include <QString>

namespace amnezia {

struct TProxyProtocolConfig {
    QString port;
    QString httpPort;
    QString secret;
    QString hostname;
    QString acmeEmail;
    QString carrierMode;
    QString workers;
    QString tgLink;
    QString tmeLink;
    bool isEnabled = true;

    QJsonObject toJson() const;
    static TProxyProtocolConfig fromJson(const QJsonObject &json);
    bool equalsDockerDeploymentSettings(const TProxyProtocolConfig &other) const;
};

} // namespace amnezia

#endif // TPROXYPROTOCOLCONFIG_H
