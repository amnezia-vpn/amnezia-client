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

namespace amnezia::gateway_sdk
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

private:

    std::shared_ptr<amnezia::gateway_sdk::GatewayController> m_controller;
};

#endif
