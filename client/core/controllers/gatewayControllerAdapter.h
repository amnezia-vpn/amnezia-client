#ifndef GATEWAYCONTROLLERADAPTER_H
#define GATEWAYCONTROLLERADAPTER_H

#include <functional>
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

class GatewayControllerAdapter : public QObject
{
    Q_OBJECT

public:
    explicit GatewayControllerAdapter(const QString &gatewayEndpoint, const bool isDevEnvironment, const int requestTimeoutMsecs,
                               const bool isStrictKillSwitchEnabled, QObject *parent = nullptr);

    amnezia::ErrorCode post(const QString &endpoint, const QJsonObject apiPayload, QByteArray &responseBody);
    QFuture<QPair<amnezia::ErrorCode, QByteArray>> postAsync(const QString &endpoint, const QJsonObject apiPayload);

    // --- Tier 2 (Шаг 6): типизированные методы поверх agw::api::* ---------------
    // Все вызываются как sync, но UI-поток не блокируют (работа на пуле + прокачка QEventLoop).
    amnezia::ErrorCode getServices(const QString &osVersion, const QString &appVersion, const QString &cliName,
                                   const QString &appLanguage, QJsonObject &servicesOut);

private:
    // Исполняет work на фоновом потоке, прокачивая локальный QEventLoop (как post). UI остаётся живым.
    void runBlocking(const std::function<void()> &work);

    std::shared_ptr<agw::GatewayController> m_controller;
};

#endif
