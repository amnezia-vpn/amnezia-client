#pragma once

#include <QScopedPointer>
#include <QTimer>
#include <QUdpSocket>

#include "core/utils/domainRouting.h"
#include "vpnProtocol.h"
#include "wireGuardProtocol.h"

/// <summary>Применяет доменные правила поверх штатного туннеля WireGuard или AmneziaWG.</summary>
class DomainRoutingProtocol final : public VpnProtocol
{
    Q_OBJECT

public:
    /// <summary>Создаёт контроллер доменной маршрутизации.</summary>
    /// <param name="configuration">Конфигурация штатного VPN и правил маршрутизации.</param>
    /// <param name="parent">Родительский объект.</param>
    explicit DomainRoutingProtocol(const QJsonObject &configuration, QObject *parent = nullptr);
    /// <summary>Освобождает принадлежащие подключению ресурсы.</summary>
    ~DomainRoutingProtocol() override;
    /// <inheritdoc/>
    ErrorCode start() override;
    /// <inheritdoc/>
    void stop() override;

private:
    /// <summary>Подготавливает исходящие пути без изменения сетевых настроек.</summary>
    /// <returns>Код результата проверки конфигурации.</returns>
    ErrorCode preparePaths();
    /// <summary>Запускает доменный обработчик после появления интерфейса VPN.</summary>
    void checkTunnel();
    /// <summary>Проверяет владение работающим обработчиком.</summary>
    void checkEngine();
    /// <summary>Завершает подключение после ошибки.</summary>
    /// <param name="error">Код обнаруженной ошибки.</param>
    void fail(ErrorCode error);

    QScopedPointer<WireguardProtocol> m_wireguard;
    amnezia::domainRouting::Options m_options;
    QTimer m_tunnelTimer;
    QTimer m_engineTimer;
    QUdpSocket m_handshakeProbe;
    qint64 m_engineToken = 0;
    bool m_tunnelConnected = false;
};
