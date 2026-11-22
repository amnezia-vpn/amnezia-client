#include "splitTunnelingUpdateController.h"

#include <QJsonArray>

#include "core/utils/constants/configKeys.h"

using namespace amnezia;

SplitTunnelingUpdateController::SplitTunnelingUpdateController(SplitTunnelingUpdateBackend *backend, QObject *parent)
    : QObject(parent), m_backend(backend), m_timeoutTimer(this)
{
    qRegisterMetaType<SplitTunnelingUpdateResult>();
    m_timeoutTimer.setSingleShot(true);
    m_timeoutTimer.setInterval(15000);
    connect(&m_timeoutTimer, &QTimer::timeout, this, [this]() {
        cancel();
        emit updateFailed(SplitTunnelingUpdateResult::StateUnknown);
    });
    if (m_backend) {
        connect(m_backend, &SplitTunnelingUpdateBackend::finished, this,
                &SplitTunnelingUpdateController::completeUpdate, Qt::QueuedConnection);
        connect(m_backend, &QObject::destroyed, this, [this]() {
            const bool wasUpdating = m_updateInProgress;
            cancel();
            if (wasUpdating) {
                emit updateFailed(SplitTunnelingUpdateResult::StateUnknown);
            }
        });
    }
}

void SplitTunnelingUpdateController::startSession(const QJsonObject &configuration)
{
    cancel();
    if (!m_backend || !m_backend->supportsConfiguration(configuration)) {
        return;
    }
    m_appliedConfiguration = configuration;
    m_requestedConfiguration = configuration;
    m_sessionActive = true;
    emit availabilityChanged();
}

void SplitTunnelingUpdateController::requestUpdate(const QJsonObject &configuration)
{
    if (!isAvailable()) {
        return;
    }
    if (!m_backend->supportsConfiguration(configuration)) {
        emit updateFailed(SplitTunnelingUpdateResult::NotApplied);
        return;
    }
    auto previous = m_requestedConfiguration;
    auto requested = configuration;
    const int previousMode = previous.take(configKey::splitTunnelType).toInt();
    const int requestedMode = requested.take(configKey::splitTunnelType).toInt();
    const auto previousSites = previous.take(configKey::splitTunnelSites).toArray();
    const auto requestedSites = requested.take(configKey::splitTunnelSites).toArray();
    if (previous != requested || (previousMode != 0 && previousMode != requestedMode)) {
        emit updateFailed(SplitTunnelingUpdateResult::NotApplied);
        return;
    }
    for (const auto &site : previousSites) {
        if (!requestedSites.contains(site)) {
            emit updateFailed(SplitTunnelingUpdateResult::NotApplied);
            return;
        }
    }
    m_requestedConfiguration = configuration;
    applyNextUpdate();
}

void SplitTunnelingUpdateController::applyNextUpdate()
{
    if (!isAvailable() || m_updateInProgress || m_requestedConfiguration == m_appliedConfiguration) {
        return;
    }
    m_pendingConfiguration = m_requestedConfiguration;
    m_updateInProgress = true;
    m_timeoutTimer.start();
    m_backend->apply(++m_requestId, m_pendingConfiguration);
}

void SplitTunnelingUpdateController::completeUpdate(quint64 requestId, SplitTunnelingUpdateResult result)
{
    if (!m_sessionActive || !m_updateInProgress || requestId != m_requestId) {
        return;
    }
    m_timeoutTimer.stop();
    if (result != SplitTunnelingUpdateResult::Applied && result != SplitTunnelingUpdateResult::NotApplied) {
        cancel();
        emit updateFailed(SplitTunnelingUpdateResult::StateUnknown);
        return;
    }
    const bool hasNewerRequest = m_requestedConfiguration != m_pendingConfiguration;
    m_updateInProgress = false;
    if (result == SplitTunnelingUpdateResult::Applied) {
        m_appliedConfiguration = m_pendingConfiguration;
        emit configurationApplied(m_appliedConfiguration);
    } else {
        emit updateFailed(result);
    }
    if (m_sessionActive && requestId == m_requestId
        && (result == SplitTunnelingUpdateResult::Applied || hasNewerRequest)) {
        applyNextUpdate();
    }
}

bool SplitTunnelingUpdateController::isAvailable() const
{
    return m_sessionActive && m_backend;
}

void SplitTunnelingUpdateController::cancel()
{
    ++m_requestId;
    m_timeoutTimer.stop();
    m_sessionActive = false;
    m_updateInProgress = false;
    m_appliedConfiguration = { };
    m_requestedConfiguration = { };
    m_pendingConfiguration = { };
    emit availabilityChanged();
}
