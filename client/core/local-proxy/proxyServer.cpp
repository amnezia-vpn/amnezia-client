#include "proxyServer.h"

#include "core/repositories/secureAppSettingsRepository.h"
#include "core/repositories/secureServersRepository.h"
#include "localProxyDefs.h"

#include <QDebug>

using namespace amnezia;

ProxyServer::ProxyServer(SecureServersRepository *serversRepository, SecureAppSettingsRepository *appSettingsRepository,
                         QObject *parent)
    : QObject(parent),
      m_appSettingsRepository(appSettingsRepository),
      m_service(new ProxyService(serversRepository, appSettingsRepository))
{
    connect(m_service.data(), &ProxyService::xrayStatusChanged, this,
            [this](bool running, int port) { emit activePortChanged(running ? port : 0); });
}

ProxyServer::~ProxyServer()
{
    stop();
}

void ProxyServer::applySettings()
{
    if (!m_appSettingsRepository || !m_appSettingsRepository->isLocalProxyHttpEnabled()) {
        qDebug() << "Local proxy is disabled";
        stop();
        return;
    }

    if (!start(localProxy::apiPort)) {
        disableWithError(tr("Local proxy failed to start. Check if the port is available."));
        return;
    }

    if (!syncXray()) {
        disableWithError(tr("Couldn’t start the proxy due to an internal error. Try restarting the app."));
        return;
    }

    qDebug() << "Local proxy is running on 127.0.0.1:" << m_currentApiPort;
}

void ProxyServer::onServerEdited(const QString &serverId)
{
    if (!m_isRunning || !m_appSettingsRepository || m_appSettingsRepository->localProxyOwnerId() != serverId) {
        return;
    }

    qDebug() << "Owner server edited, restarting Xray";
    if (m_service->restartXray()) {
        m_currentProxyPort = m_appSettingsRepository->localProxyPort();
    } else {
        disableWithError(tr("Couldn’t start the proxy due to an internal error. Try restarting the app."));
    }
}

void ProxyServer::onServerRemoved(const QString &serverId)
{
    if (!m_appSettingsRepository || m_appSettingsRepository->localProxyOwnerId() != serverId) {
        return;
    }

    m_appSettingsRepository->setLocalProxyOwnerId(QString());
    m_appSettingsRepository->setLocalProxyHttpEnabled(false);
}

bool ProxyServer::start(quint16 apiPort)
{
    if (m_isRunning && m_currentApiPort == apiPort) {
        return true;
    }

    if (m_isRunning) {
        stop();
    }

    m_api.reset(new HttpApi(m_service.toWeakRef()));
    if (!m_api->start(apiPort)) {
        m_api.reset();
        return false;
    }

    m_isRunning = true;
    m_currentApiPort = apiPort;
    return true;
}

void ProxyServer::stop()
{
    m_service->stopXray();
    if (m_api) {
        m_api->stop();
        m_api.reset();
    }
    m_isRunning = false;
    m_currentApiPort = 0;
    m_currentProxyPort = 0;
}

bool ProxyServer::syncXray()
{
    const quint16 proxyPort = m_appSettingsRepository->localProxyPort();

    bool synced;
    if (!m_service->isXrayRunning()) {
        synced = m_service->startXray();
    } else if (m_currentProxyPort != proxyPort) {
        qDebug() << "Proxy port changed from" << m_currentProxyPort << "to" << proxyPort;
        synced = m_service->restartXray();
    } else {
        return true;
    }

    if (synced) {
        m_currentProxyPort = proxyPort;
    }
    return synced;
}

void ProxyServer::disableWithError(const QString &message)
{
    qWarning() << message;

    if (m_appSettingsRepository && m_appSettingsRepository->isLocalProxyHttpEnabled()) {
        m_appSettingsRepository->setLocalProxyHttpEnabled(false);
    } else {
        stop();
    }

    emit startFailed(message);
}
