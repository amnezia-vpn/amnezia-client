#pragma once

#include "iproxyservice.h"
#include "configmanager.h"
#include "xraycontroller.h"
#include <QObject>
#include <QScopedPointer>

class ProxyService : public QObject, public IProxyService {
    Q_OBJECT

public:
    explicit ProxyService(QObject* parent = nullptr);
    ~ProxyService() = default;

    QJsonObject getConfig() const override;
    bool updateConfig(const QString& configStr) override;
    QMap<QString, QJsonObject> getAllConfigs() const override;
    QMap<QString, QJsonObject> getConfigsByUuids(const QStringList &uuids) const override;
    bool addConfigs(const QStringList &serializedConfigs) override;
    bool removeConfig(const QString &uuid) override;
    bool activateConfig(const QString &uuid) override;
    QJsonObject getActiveConfig() const override;
    bool updateAllConfigs(const QStringList &serializedConfigs) override;
    QString getActiveConfigUuid() const;
    int getConfigCount() const override;

    bool startXray() override;
    bool stopXray() override;
    bool isXrayRunning() const override;
    qint64 getXrayProcessId() const override;
    QString getXrayError() const override;

signals:
    void configsChanged();
    void xrayStatusChanged(bool running);

private:
    QScopedPointer<ConfigManager> m_configManager;
    QScopedPointer<XrayController> m_xrayController;
}; 