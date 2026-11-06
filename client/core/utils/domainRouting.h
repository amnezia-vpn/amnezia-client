#pragma once

#include <QJsonObject>
#include <QStringList>

#include <optional>

#include "core/utils/routeModes.h"

namespace amnezia::domainRouting
{
    enum class RuleKind {
        Domain,
        Subdomains,
        Network
    };

    struct Rule
    {
        RuleKind kind;
        QString value;
    };

    struct Path
    {
        QString interfaceName;
        QString sourceAddress;
        QStringList dnsServers;
    };

    struct Options
    {
        RouteMode mode = RouteMode::VpnAllSites;
        QStringList rules;
        AppsRouteMode appsMode = AppsRouteMode::VpnAllApps;
        QStringList apps;
        Path direct;
        Path tunnel;
        QJsonObject hosts;
        quint16 socksPort = 0;
        quint16 dnsPort = 0;
        QString socksUser;
        QString socksPassword;
        QString tunName;
        quint16 mtu = 1420;
    };

    /// <summary>Разбирает правило доменной маршрутизации.</summary>
    /// <param name="text">Домен, маска поддоменов, адрес или подсеть.</param>
    /// <returns>Нормализованное правило либо пустой результат при ошибке.</returns>
    std::optional<Rule> parseRule(const QString &text);

    /// <summary>Создаёт конфигурацию локального обработчика доменных маршрутов.</summary>
    /// <param name="options">Правила, пути исходящих соединений и параметры локальных слушателей.</param>
    /// <param name="error">Описание ошибки, очищаемое при успешной проверке.</param>
    /// <returns>Конфигурация Xray без параметров удалённого VPN-сервера либо пустой объект при ошибке.</returns>
    QJsonObject buildConfig(const Options &options, QString &error);
}
