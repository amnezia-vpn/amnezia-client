#pragma once

#include <QJsonObject>
#include <QMap>

class IProxyService {
public:
    virtual ~IProxyService() = default;

    // Config operations
    virtual QJsonObject getConfig() const = 0;
    virtual bool updateConfig(const QString& configStr) = 0;
    virtual QMap<QString, QJsonObject> getAllConfigs() const = 0;
    virtual QMap<QString, QJsonObject> getConfigsByUuids(const QStringList &uuids) const = 0;
    virtual bool addConfigs(const QStringList &serializedConfigs) = 0;
    virtual bool removeConfig(const QString &uuid) = 0;
    virtual bool activateConfig(const QString &uuid) = 0;
    virtual QJsonObject getActiveConfig() const = 0;
    virtual bool updateAllConfigs(const QStringList &serializedConfigs) = 0;
    virtual int getConfigCount() const = 0;

    // Xray process operations
    virtual bool startXray() = 0;
    virtual bool stopXray() = 0;
    virtual bool isXrayRunning() const = 0;
    virtual qint64 getXrayProcessId() const = 0;
    virtual QString getXrayError() const = 0;
}; 