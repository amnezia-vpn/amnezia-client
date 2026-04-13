#pragma once

#include <QObject>
#include <QScopedPointer>
#include <QSharedPointer>
#include <memory>

#include "httpapi.h"
#include "proxyservice.h"

class Settings;

class ProxyServer : public QObject
{
    Q_OBJECT

public:
    explicit ProxyServer(const std::shared_ptr<Settings> &settings, QObject *parent = nullptr);
    ~ProxyServer();

    bool start(quint16 port = 49490);
    void stop();
    bool syncSettings();

private:
    bool startXrayProcess();
    void stopXrayProcess();

    std::shared_ptr<Settings> m_settings;
    QScopedPointer<HttpApi> m_api;
    QSharedPointer<ProxyService> m_service;
    bool m_isRunning {false};
    quint16 m_currentApiPort {0};
    quint16 m_currentProxyPort {0};
    int m_lastRestartToken {0};
};