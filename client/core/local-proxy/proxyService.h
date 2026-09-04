#ifndef PROXYSERVICE_H
#define PROXYSERVICE_H

#include <QJsonObject>
#include <QObject>

#include "proxyConfigManager.h"
#include "xrayEngineClient.h"

class SecureServersRepository;
class SecureAppSettingsRepository;

class ProxyService : public QObject
{
    Q_OBJECT

public:
    ProxyService(SecureServersRepository *serversRepository, SecureAppSettingsRepository *appSettingsRepository,
                 QObject *parent = nullptr);

    QJsonObject config() const;
    bool startXray();
    bool stopXray();
    bool restartXray();
    bool isXrayRunning() const;

signals:
    void xrayStatusChanged(bool running);

private:
    ProxyConfigManager m_configManager;
    XrayEngineClient m_engine;
    QJsonObject m_cachedConfig;
};

#endif // PROXYSERVICE_H
