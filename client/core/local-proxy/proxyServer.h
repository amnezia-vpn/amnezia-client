#ifndef PROXYSERVER_H
#define PROXYSERVER_H

#include <QObject>
#include <QScopedPointer>
#include <QSharedPointer>

#include "httpApi.h"
#include "proxyService.h"

class SecureServersRepository;
class SecureAppSettingsRepository;

class ProxyServer : public QObject
{
    Q_OBJECT

public:
    ProxyServer(SecureServersRepository *serversRepository, SecureAppSettingsRepository *appSettingsRepository,
                QObject *parent = nullptr);
    ~ProxyServer();

public slots:
    void applySettings();
    void onServerEdited(const QString &serverId);
    void onServerRemoved(const QString &serverId);

signals:
    void startFailed(const QString &message);
    void activePortChanged(int port);

private:
    bool start(quint16 apiPort);
    void stop();
    bool syncXray();
    void disableWithError(const QString &message);

    SecureAppSettingsRepository *m_appSettingsRepository;
    QScopedPointer<HttpApi> m_api;
    QSharedPointer<ProxyService> m_service;
    bool m_isRunning = false;
    quint16 m_currentApiPort = 0;
    quint16 m_currentProxyPort = 0;
};

#endif // PROXYSERVER_H
