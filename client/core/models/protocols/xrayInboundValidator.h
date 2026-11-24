#ifndef XRAYINBOUNDVALIDATOR_H
#define XRAYINBOUNDVALIDATOR_H

#include <QJsonObject>
#include <QList>
#include <QString>

namespace amnezia
{

    struct XrayInboundIssue
    {
        enum class Level {
            Error,
            Info,
        };

        Level level = Level::Error;
        int line = 1;
        int column = 0;
        QString message;
    };

    struct XrayInboundValidation
    {
        bool parsed = false;
        QJsonObject inbound;
        QList<XrayInboundIssue> issues;

        bool isValid() const;
        const XrayInboundIssue *firstError() const;
    };

    XrayInboundValidation validateXrayInbound(const QString &text);

} // namespace amnezia

#endif // XRAYINBOUNDVALIDATOR_H
