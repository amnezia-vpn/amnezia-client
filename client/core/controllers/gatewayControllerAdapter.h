#ifndef GATEWAYCONTROLLERADAPTER_H
#define GATEWAYCONTROLLERADAPTER_H

#include <memory>

#include <QByteArray>
#include <QFuture>
#include <QJsonObject>
#include <QObject>
#include <QPair>
#include <QString>

#include "core/utils/errorCodes.h"

namespace agw
{
class GatewayController;
}

// Тонкий Qt-адаптер над agw::GatewayController (agw-sdk). Сигнатуры — как раньше, вызывающий код не
// меняется. Транспорт/крипта/failover живут в SDK. См. docs/plans/gateway-sdk/agw-sdk-tier1-phase6-integration.md
class GatewayControllerAdapter : public QObject
{
    Q_OBJECT

public:
    explicit GatewayControllerAdapter(const QString &gatewayEndpoint, const bool isDevEnvironment, const int requestTimeoutMsecs,
                               const bool isStrictKillSwitchEnabled, QObject *parent = nullptr);

    amnezia::ErrorCode post(const QString &endpoint, const QJsonObject apiPayload, QByteArray &responseBody);
    QFuture<QPair<amnezia::ErrorCode, QByteArray>> postAsync(const QString &endpoint, const QJsonObject apiPayload);

private:
    // Долгоживущий клиент окружения (кеш прокси переживает запросы). Берётся из статического реестра.
    std::shared_ptr<agw::GatewayController> m_controller;
};

#endif // GATEWAYCONTROLLERADAPTER_H
