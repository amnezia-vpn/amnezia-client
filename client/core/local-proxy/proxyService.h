#ifndef PROXYSERVICE_H
#define PROXYSERVICE_H

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

    bool startXray();
    bool stopXray();
    bool restartXray();
    bool isXrayRunning() const;
    int activePort() const;

signals:
    void xrayStatusChanged(bool running, int port);

private:
    ProxyConfigManager m_configManager;
    XrayEngineClient m_engine;
    int m_activePort = 0;
};

#endif // PROXYSERVICE_H
