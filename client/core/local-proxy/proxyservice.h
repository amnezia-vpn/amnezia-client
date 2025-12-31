#pragma once

#include "configmanager.h"
#include "iproxyservice.h"
#include "xraycontroller.h"

#include <QObject>
#include <QScopedPointer>
#include <QJsonObject>
#include <memory>

class Settings;

class ProxyService : public QObject, public IProxyService {
    Q_OBJECT

public:
    explicit ProxyService(const std::shared_ptr<Settings> &settings, QObject* parent = nullptr);
    ~ProxyService() = default;

    QJsonObject getConfig() override;
    bool startXray() override;
    bool stopXray() override;
    bool isXrayRunning() const override;
    qint64 getXrayProcessId() const override;
    QString getXrayError() const override;

signals:
    void xrayStatusChanged(bool running);

private:
    QScopedPointer<ConfigManager> m_configManager;
    QScopedPointer<XrayController> m_xrayController;
    QJsonObject m_cachedConfig;
}; 