#pragma once

#include <QJsonObject>
#include <QString>

class IProxyService {
public:
    virtual ~IProxyService() = default;

    virtual QJsonObject getConfig() = 0;

    virtual bool startXray() = 0;
    virtual bool stopXray() = 0;
    virtual bool isXrayRunning() const = 0;
    virtual qint64 getXrayProcessId() const = 0;
    virtual QString getXrayError() const = 0;
}; 