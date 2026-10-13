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

class GatewayController : public QObject
{
    Q_OBJECT

public:
    explicit GatewayController(const QString &gatewayEndpoint, const bool isDevEnvironment, const int requestTimeoutMsecs,
                               const bool isStrictKillSwitchEnabled, SecureAppSettingsRepository *appSettingsRepository,
                               QObject *parent = nullptr);
    ~GatewayController() override;

    amnezia::ErrorCode post(const QString &endpoint, const QJsonObject apiPayload, QByteArray &responseBody);
    QFuture<QPair<amnezia::ErrorCode, QByteArray>> postAsync(const QString &endpoint, const QJsonObject apiPayload);

private:
    static void onBeforeRequest(const char *host, void *userData);
    void handleBeforeRequest(const QString &host);

    QPair<amnezia::ErrorCode, QByteArray> executePost(const QString &endpoint, const QJsonObject &apiPayload);
    static amnezia::ErrorCode mapResultCode(const int code, const QByteArray &responseBody);
    void persistState();

    bool m_isStrictKillSwitchEnabled = false;
    SecureAppSettingsRepository *m_appSettingsRepository = nullptr;

    agw_client_handle m_client = 0;
    bool m_publicKeyMissing = false;
    QByteArray m_lastPersistedState;
};

#endif // GATEWAYCONTROLLER_H
