#include "proxyserver.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>

ProxyServer::ProxyServer(QObject *parent)
    : QObject(parent)
    , m_service(new ProxyService(this))
{
    connect(m_service.data(), &ProxyService::configsChanged, this, &ProxyServer::startXrayProcess);
}

ProxyServer::~ProxyServer()
{
    stop();
}

bool ProxyServer::start(quint16 port)
{
    m_api.reset(new HttpApi(m_service.toWeakRef()));
    bool apiStarted = m_api->start(port);
    if (!apiStarted) {
        qWarning() << "Local proxy: port is busy:" << port;
        return false;
    }

    // Auto-start Xray if config exists
    QJsonObject config = m_service->getConfig();
    if (!config.isEmpty()) {
        startXrayProcess();
    } else {
        qDebug() << "No config found, Xray will not start automatically";
    }

    return true;
}

void ProxyServer::stop()
{
    stopXrayProcess();
    if (m_api) {
        m_api->stop();
    }
}

bool ProxyServer::startXrayProcess()
{
    return m_service->startXray();
}

void ProxyServer::stopXrayProcess()
{
    m_service->stopXray();
}