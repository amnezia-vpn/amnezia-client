#ifndef SPLITTUNNELINGUPDATECONTROLLER_H
#define SPLITTUNNELINGUPDATECONTROLLER_H

#include <QJsonObject>
#include <QObject>
#include <QPointer>
#include <QTimer>

#include "core/splitTunneling/splitTunnelingUpdateBackend.h"

class SplitTunnelingUpdateController : public QObject
{
    Q_OBJECT

public:
    explicit SplitTunnelingUpdateController(SplitTunnelingUpdateBackend *backend, QObject *parent = nullptr);

    void startSession(const QJsonObject &configuration);
    void requestUpdate(const QJsonObject &configuration);
    bool isAvailable() const;
    void cancel();

signals:
    void configurationApplied(const QJsonObject &configuration);
    void updateFailed(SplitTunnelingUpdateResult result);
    void availabilityChanged();

private:
    void applyNextUpdate();
    void completeUpdate(quint64 requestId, SplitTunnelingUpdateResult result);

    QPointer<SplitTunnelingUpdateBackend> m_backend;
    QTimer m_timeoutTimer;
    QJsonObject m_appliedConfiguration;
    QJsonObject m_requestedConfiguration;
    QJsonObject m_pendingConfiguration;
    quint64 m_requestId = 0;
    bool m_sessionActive = false;
    bool m_updateInProgress = false;
};

#endif
