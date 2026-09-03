#ifndef GATEWAYCONTROLLER_H
#define GATEWAYCONTROLLER_H

#include <QByteArray>
#include <QFuture>
#include <QJsonObject>
#include <QObject>
#include <QPair>
#include <QString>

#include "core/utils/errorCodes.h"

#include "agw.h"

class SecureAppSettingsRepository;

// Thin Qt facade over the libagw transport: the envelope crypto, HTTP and
// proxy failover live in the Go library; this class
// keeps the historical GatewayController API for the existing call sites and
// wires the host-side concerns (killswitch exceptions, iOS inet access, state
// persistence) into the library callbacks.
class GatewayController : public QObject
{
    Q_OBJECT

public:
    explicit GatewayController(const QString &gatewayEndpoint, const bool isDevEnvironment, const int requestTimeoutMsecs,
                               const bool isStrictKillSwitchEnabled, SecureAppSettingsRepository *appSettingsRepository,
                               QObject *parent = nullptr);
    ~GatewayController() override;

    // Blocks like the old implementation did: pumps a local event loop
    // (ExcludeUserInputEvents) until the request completes, so UI-thread
    // callers keep their behaviour.
    amnezia::ErrorCode post(const QString &endpoint, const QJsonObject apiPayload, QByteArray &responseBody);

    // Runs the request on a worker thread. The controller must outlive the
    // returned future (call sites hold it in a QSharedPointer).
    QFuture<QPair<amnezia::ErrorCode, QByteArray>> postAsync(const QString &endpoint, const QJsonObject apiPayload);

    // Internal: invoked from the library's on_before_request callback,
    // marshalled to this object's thread. Public only for the C trampoline.
    void handleBeforeRequest(const QString &host);

private:
    QPair<amnezia::ErrorCode, QByteArray> executePost(const QString &endpoint, const QJsonObject &apiPayload);
    void persistState();

    bool m_isStrictKillSwitchEnabled = false;
    SecureAppSettingsRepository *m_appSettingsRepository = nullptr;

    agw_client_handle m_client = 0;
    bool m_publicKeyMissing = false;
    QByteArray m_lastPersistedState;
};

#endif // GATEWAYCONTROLLER_H
