#include <winsock2.h>

#include "platforms/windows/daemon/windowsroutemonitor.h"

#include <QTest>

#include <cstdlib>

class TestRouteLogger
{
public:
    /// <summary>Создаёт вывод диагностик изолированного теста.</summary>
    /// <param name="name">Имя проверяемого компонента.</param>
    explicit TestRouteLogger(const QString &name)
    {
        Q_UNUSED(name);
    }
    /// <summary>Создаёт поток диагностических сообщений.</summary>
    /// <returns>Диагностический поток тестового процесса.</returns>
    QDebug debug()
    {
        return qDebug();
    }
    /// <summary>Создаёт поток предупреждений.</summary>
    /// <returns>Диагностический поток тестового процесса.</returns>
    QDebug warning()
    {
        return qDebug();
    }
    /// <summary>Создаёт поток ошибок.</summary>
    /// <returns>Диагностический поток тестового процесса.</returns>
    QDebug error()
    {
        return qDebug();
    }
};

namespace
{
    QList<MIB_IPFORWARD_ROW2> routes;
    quint64 overlayLuid = 0;
    bool physicalConnected = true;
    constexpr quint64 physicalLuid = 10;
    constexpr quint64 tunnelLuid = 20;

    /// <summary>Создаёт тестовый IPv4-маршрут.</summary>
    /// <param name="prefix">Подсеть назначения.</param>
    /// <param name="luid">Идентификатор интерфейса.</param>
    /// <returns>Строка изолированной таблицы маршрутов.</returns>
    MIB_IPFORWARD_ROW2 route(const QString &prefix, quint64 luid)
    {
        const IPAddress address(prefix);
        MIB_IPFORWARD_ROW2 result = { };
        result.InterfaceLuid.Value = luid;
        result.DestinationPrefix.Prefix.Ipv4.sin_family = AF_INET;
        result.DestinationPrefix.Prefix.Ipv4.sin_addr.s_addr = htonl(address.address().toIPv4Address());
        result.DestinationPrefix.PrefixLength = address.prefixLength();
        result.NextHop.si_family = AF_INET;
        result.Metric = 1;
        result.Protocol = MIB_IPPROTO_NETMGMT;
        return result;
    }

    /// <summary>Подменяет поиск интерфейса доменного обработчика.</summary>
    /// <param name="alias">Проверяемое имя интерфейса.</param>
    /// <param name="luid">Полученный идентификатор.</param>
    /// <returns>Код результата поиска.</returns>
    DWORD aliasToLuid(const wchar_t *alias, NET_LUID *luid)
    {
        if (QString::fromWCharArray(alias) != QStringLiteral("AmneziaDomains") || !overlayLuid) {
            return ERROR_NOT_FOUND;
        }
        luid->Value = overlayLuid;
        return NO_ERROR;
    }

    /// <summary>Возвращает изолированный список сетевых интерфейсов.</summary>
    /// <param name="family">Запрошенное семейство адресов.</param>
    /// <param name="table">Выделенная таблица интерфейсов.</param>
    /// <returns>Код успешного выполнения.</returns>
    DWORD interfaceTable(ADDRESS_FAMILY family, MIB_IPINTERFACE_TABLE **table)
    {
        Q_UNUSED(family);
        *table = static_cast<MIB_IPINTERFACE_TABLE *>(
                std::calloc(1, sizeof(MIB_IPINTERFACE_TABLE) + 2 * sizeof(MIB_IPINTERFACE_ROW)));
        (*table)->NumEntries = overlayLuid ? 3 : 2;
        const quint64 ids[] = { physicalLuid, tunnelLuid, overlayLuid };
        for (ULONG i = 0; i < (*table)->NumEntries; ++i) {
            MIB_IPINTERFACE_ROW &row = (*table)->Table[i];
            row.Family = AF_INET;
            row.InterfaceLuid.Value = ids[i];
            row.Connected = i != 0 || physicalConnected;
        }
        return NO_ERROR;
    }

    /// <summary>Возвращает копию изолированной таблицы маршрутов.</summary>
    /// <param name="family">Запрошенное семейство адресов.</param>
    /// <param name="table">Выделенная таблица маршрутов.</param>
    /// <returns>Код успешного выполнения.</returns>
    DWORD routeTable(ADDRESS_FAMILY family, MIB_IPFORWARD_TABLE2 **table)
    {
        Q_UNUSED(family);
        *table = static_cast<MIB_IPFORWARD_TABLE2 *>(
                std::calloc(1, sizeof(MIB_IPFORWARD_TABLE2) + routes.size() * sizeof(MIB_IPFORWARD_ROW2)));
        (*table)->NumEntries = static_cast<ULONG>(routes.size());
        for (qsizetype i = 0; i < routes.size(); ++i) {
            (*table)->Table[i] = routes.at(i);
        }
        return NO_ERROR;
    }

    /// <summary>Добавляет маршрут только в тестовую таблицу.</summary>
    /// <param name="row">Добавляемая строка.</param>
    /// <returns>Код успешного выполнения.</returns>
    DWORD createRoute(const MIB_IPFORWARD_ROW2 *row)
    {
        routes.append(*row);
        return NO_ERROR;
    }

    /// <summary>Удаляет маршрут только из тестовой таблицы.</summary>
    /// <param name="row">Удаляемая строка.</param>
    /// <returns>Код результата удаления.</returns>
    DWORD deleteRoute(const MIB_IPFORWARD_ROW2 *row)
    {
        for (qsizetype i = 0; i < routes.size(); ++i) {
            const MIB_IPFORWARD_ROW2 &candidate = routes.at(i);
            if (candidate.InterfaceLuid.Value == row->InterfaceLuid.Value
                && candidate.DestinationPrefix.PrefixLength == row->DestinationPrefix.PrefixLength
                && candidate.DestinationPrefix.Prefix.Ipv4.sin_addr.s_addr
                        == row->DestinationPrefix.Prefix.Ipv4.sin_addr.s_addr) {
                routes.removeAt(i);
                return NO_ERROR;
            }
        }
        return ERROR_NOT_FOUND;
    }

    /// <summary>Подменяет регистрацию системного обработчика изменений.</summary>
    /// <param name="family">Запрошенное семейство адресов.</param>
    /// <param name="callback">Обработчик уведомлений.</param>
    /// <param name="context">Контекст обработчика.</param>
    /// <param name="initial">Признак начального уведомления.</param>
    /// <param name="handle">Тестовый дескриптор регистрации.</param>
    /// <returns>Код успешного выполнения.</returns>
    DWORD notifyRoutes(ADDRESS_FAMILY family, PIPFORWARD_CHANGE_CALLBACK callback, void *context, BOOLEAN initial,
                       HANDLE *handle)
    {
        Q_UNUSED(family);
        Q_UNUSED(callback);
        Q_UNUSED(context);
        Q_UNUSED(initial);
        *handle = nullptr;
        return NO_ERROR;
    }

    /// <summary>Подменяет снятие регистрации системного обработчика.</summary>
    /// <param name="handle">Тестовый дескриптор регистрации.</param>
    /// <returns>Код успешного выполнения.</returns>
    DWORD cancelNotifications(HANDLE handle)
    {
        Q_UNUSED(handle);
        return NO_ERROR;
    }

    /// <summary>Определяет интерфейс исключения для тестового VPN-сервера.</summary>
    /// <returns>Идентификатор интерфейса либо ноль при отсутствии маршрута.</returns>
    quint64 endpointInterface()
    {
        for (const MIB_IPFORWARD_ROW2 &row : routes) {
            if (row.DestinationPrefix.PrefixLength == 32) {
                return row.InterfaceLuid.Value;
            }
        }
        return 0;
    }
}

#define ConvertInterfaceAliasToLuid aliasToLuid
#define GetIpInterfaceTable         interfaceTable
#define GetIpForwardTable2          routeTable
#define CreateIpForwardEntry2       createRoute
#define DeleteIpForwardEntry2       deleteRoute
#define FreeMibTable                std::free
#define NotifyRouteChange2          notifyRoutes
#define CancelMibChangeNotify2      cancelNotifications
#define LOGGER_H
#define Logger TestRouteLogger
#include "../../platforms/windows/daemon/windowsroutemonitor.cpp"
#undef Logger
#undef LOGGER_H
#undef ConvertInterfaceAliasToLuid
#undef GetIpInterfaceTable
#undef GetIpForwardTable2
#undef CreateIpForwardEntry2
#undef DeleteIpForwardEntry2
#undef FreeMibTable
#undef NotifyRouteChange2
#undef CancelMibChangeNotify2

class TestDomainRouteMonitor : public QObject
{
    Q_OBJECT

private slots:
    /// <summary>Восстанавливает изолированную таблицу перед каждым тестом.</summary>
    void init()
    {
        overlayLuid = 0;
        physicalConnected = true;
        routes = { route(QStringLiteral("0.0.0.0/0"), physicalLuid), route(QStringLiteral("0.0.0.0/0"), tunnelLuid) };
    }

    /// <summary>Проверяет появление, пересоздание и удаление доменного интерфейса.</summary>
    void overlayLifecycle()
    {
        WindowsRouteMonitor monitor(tunnelLuid, nullptr, QStringLiteral("AmneziaDomains"));
        QVERIFY(monitor.addExclusionRoute(IPAddress(QStringLiteral("203.0.113.7/32"))));
        QCOMPARE(endpointInterface(), physicalLuid);
        for (quint64 id : { 30, 31 }) {
            overlayLuid = id;
            routes.append(route(QStringLiteral("128.0.0.0/1"), id));
            monitor.routeChanged();
            QCOMPARE(endpointInterface(), physicalLuid);
        }
        overlayLuid = 0;
        monitor.routeChanged();
        QCOMPARE(endpointInterface(), physicalLuid);
    }

    /// <summary>Проверяет отсутствие перехода на доменный интерфейс при потере физической сети.</summary>
    void physicalPathLost()
    {
        overlayLuid = 30;
        routes.append(route(QStringLiteral("128.0.0.0/1"), overlayLuid));
        WindowsRouteMonitor monitor(tunnelLuid, nullptr, QStringLiteral("AmneziaDomains"));
        QVERIFY(monitor.addExclusionRoute(IPAddress(QStringLiteral("203.0.113.7/32"))));
        QCOMPARE(endpointInterface(), physicalLuid);
        physicalConnected = false;
        monitor.routeChanged();
        QVERIFY(endpointInterface() == 0 || endpointInterface() == physicalLuid);
        physicalConnected = true;
        monitor.routeChanged();
        QCOMPARE(endpointInterface(), physicalLuid);
    }

    /// <summary>Проверяет сохранение прежнего выбора маршрута без доменного режима.</summary>
    void nativeModeUnchanged()
    {
        overlayLuid = 30;
        routes.append(route(QStringLiteral("128.0.0.0/1"), overlayLuid));
        WindowsRouteMonitor monitor(tunnelLuid, nullptr);
        QVERIFY(monitor.addExclusionRoute(IPAddress(QStringLiteral("203.0.113.7/32"))));
        QCOMPARE(endpointInterface(), overlayLuid);
    }
};

QTEST_GUILESS_MAIN(TestDomainRouteMonitor)
#include "testDomainRouteMonitor.moc"
