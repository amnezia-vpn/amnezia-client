#include "serverLatencyUiController.h"

#include <QQmlEngine>

#include "core/controllers/serversController.h"
#include "ui/models/serversModel.h"

ServerLatencyUiController::ServerLatencyUiController(ServerLatencyController *latencyController,
                                                     ServersController *serversController, ServersModel *serversModel,
                                                     QObject *parent)
    : QObject(parent),
      m_latencyController(latencyController),
      m_serversController(serversController),
      m_serversModel(serversModel)
{
    connect(m_latencyController, &ServerLatencyController::testingChanged, this,
            &ServerLatencyUiController::testingChanged);
    connect(m_latencyController, &ServerLatencyController::serverLatencyChanged, this,
            &ServerLatencyUiController::onServerLatencyChanged);
}

void ServerLatencyUiController::declareQmlServerQualityEnum()
{
    qmlRegisterUncreatableMetaObject(ServerQuality::staticMetaObject, "ServerQuality", 1, 0, "ServerQuality",
                                     "Error: only enums");
}

bool ServerLatencyUiController::isTesting() const
{
    return m_latencyController->isTesting();
}

int ServerLatencyUiController::defaultServerLatencyMs() const
{
    return defaultServerResult().latencyMs;
}

int ServerLatencyUiController::defaultServerQuality() const
{
    return static_cast<int>(defaultServerResult().quality);
}

bool ServerLatencyUiController::isDefaultServerTesting() const
{
    return m_latencyController->isTesting(m_serversController->getDefaultServerId());
}

void ServerLatencyUiController::testAllServers()
{
    m_latencyController->testAllServers();
}

void ServerLatencyUiController::testDefaultServer()
{
    m_latencyController->testDefaultServer();
}

void ServerLatencyUiController::onDefaultServerChanged()
{
    emit defaultServerLatencyChanged();
    testDefaultServer();
}

void ServerLatencyUiController::onServerLatencyChanged(const QString &serverId)
{
    m_serversModel->setServerLatency(serverId, m_latencyController->result(serverId).value_or(LatencyResult { }),
                                     m_latencyController->isTesting(serverId));
    if (serverId == m_serversController->getDefaultServerId()) {
        emit defaultServerLatencyChanged();
    }
}

LatencyResult ServerLatencyUiController::defaultServerResult() const
{
    return m_latencyController->result(m_serversController->getDefaultServerId()).value_or(LatencyResult { });
}
