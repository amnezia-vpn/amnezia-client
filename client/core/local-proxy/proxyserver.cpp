#include "proxyserver.h"
#include "settings.h"

#include <QDebug>

ProxyServer::ProxyServer(const std::shared_ptr<Settings> &settings, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_service(new ProxyService(settings, this))
{
}

ProxyServer::~ProxyServer()
{
    stop();
}

bool ProxyServer::start(quint16 port)
{
    if (m_isRunning) {
        if (m_currentApiPort == port) {
            qInfo() << "Local proxy: already running on port" << port;
            return true;
        }

        qInfo() << "Local proxy: restarting on new port" << port;
        stop();
    }

    m_api.reset(new HttpApi(m_service.toWeakRef()));
    const bool apiStarted = m_api->start(port);
    if (!apiStarted) {
        qWarning() << "Local proxy: port is busy:" << port;
        m_api.reset();
        m_isRunning = false;
        m_currentApiPort = 0;
        return false;
    }

    m_isRunning = true;
    m_currentApiPort = port;

    return true;
}

void ProxyServer::stop()
{
    stopXrayProcess();
    if (m_api) {
        m_api->stop();
        m_api.reset();
    }
    m_isRunning = false;
    m_currentApiPort = 0;
    m_currentProxyPort = 0;
}

bool ProxyServer::startXrayProcess()
{
    return m_service->startXray();
}

void ProxyServer::stopXrayProcess()
{
    m_service->stopXray();
}

bool ProxyServer::syncSettings()
{
    if (!m_isRunning) {
        qDebug() << "Local proxy: syncSettings called but server is not running";
        return false;
    }

    const quint16 newProxyPort = m_settings ? m_settings->localProxyPort() : 0;
    const bool xrayRunning = m_service->isXrayRunning();

    if (!xrayRunning) {
        qInfo() << "Local proxy: starting Xray on port" << newProxyPort;
        const bool started = startXrayProcess();
        if (started) {
            m_currentProxyPort = newProxyPort;
        }
        return started;
    }

    if (m_currentProxyPort != newProxyPort) {
        qInfo() << "Local proxy: proxy port changed from" << m_currentProxyPort << "to" << newProxyPort;
        const bool restarted = m_service->restartXray();
        if (restarted) {
            m_currentProxyPort = newProxyPort;
        }
        return restarted;
    }

    return true;
}