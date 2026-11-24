#ifndef XRAYINBOUND_H
#define XRAYINBOUND_H

#include <QJsonArray>
#include <QJsonObject>
#include <QString>

namespace amnezia
{

    struct XrayServerConfig;

    namespace xrayInbound
    {
        inline constexpr char autoValue[] = "@auto";

        struct Secrets
        {
            QJsonArray clients;
            QString realityPrivateKey;
            QString realityShortId;
        };

        QJsonObject parse(const QString &templateText);
        QString format(const QJsonObject &inbound);
        QString normalized(const QString &templateText);

        QJsonObject toTemplate(const QJsonObject &inbound);
        QJsonObject inboundFromServerJson(const QJsonObject &serverJson);
        QJsonObject toServerJson(const QJsonObject &inboundTemplate, const Secrets &secrets);

        QString templateFlow(const QJsonObject &inboundTemplate);
        int templatePort(const QJsonObject &inboundTemplate);

        QString templateFromServerConfig(const XrayServerConfig &serverConfig);
        bool applyTemplate(const QString &templateText, XrayServerConfig &serverConfig);
    }

} // namespace amnezia

#endif // XRAYINBOUND_H
