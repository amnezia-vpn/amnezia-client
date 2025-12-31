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

    bool start(quint16 port = 8080);
    void stop();

private:
    bool startXrayProcess();
    void stopXrayProcess();

    QScopedPointer<HttpApi> m_api;
    QSharedPointer<ProxyService> m_service;
    bool m_isRunning {false};
    quint16 m_currentPort {0};
};