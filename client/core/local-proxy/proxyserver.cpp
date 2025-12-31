#include "proxyserver.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QStandardPaths>

#include "proxylogger.h"

ProxyServer::ProxyServer(QObject *parent)
    : QObject(parent)
    , m_service(new ProxyService(this))
{
    const QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/logs";
    ProxyLogger::getInstance().init(logDir + "/proxy.log");
    ProxyLogger::getInstance().setLogLevel(ProxyLogger::LogLevel::Info);

    connect(m_service.data(), &ProxyService::configsChanged, this, &ProxyServer::startXrayProcess);
}

ProxyServer::~ProxyServer()
{
    stop();
}

bool ProxyServer::start(quint16 port)
{
    if (m_isRunning) {
        if (m_currentPort == port) {
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
        m_currentPort = 0;
        return false;
    }

    // Auto-start Xray if config exists
    QJsonObject config = m_service->getConfig();
    if (!config.isEmpty()) {
        startXrayProcess();
    } else {
        qDebug() << "No config found, Xray will not start automatically";
    }

    m_isRunning = true;
    m_currentPort = port;

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
    m_currentPort = 0;
}

bool ProxyServer::startXrayProcess()
{
    return m_service->startXray();
}

void ProxyServer::stopXrayProcess()
{
    m_service->stopXray();
}