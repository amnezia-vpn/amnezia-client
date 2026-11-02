#ifndef XRAY_H
#define XRAY_H

#include <QString>
#include <QTimer>

class Xray
{
public:
    static Xray& getInstance()
    {
        static Xray instance;
        return instance;
    }

    // The engine is shared between consumers (VPN protocol, local proxy).
    // start() returns an owner token; stop(token) is a no-op unless the token
    // still owns the running engine, so a late teardown can't kill a newer owner.
    /// <summary>Запускает обработчик с контролем владения.</summary>
    /// <param name="cfg">Конфигурация обработчика.</param>
    /// <param name="exclusive">Запрет замены обработчика другим потребителем.</param>
    /// <returns>Токен владельца либо ноль при отказе.</returns>
    qint64 start(const QString &cfg, bool exclusive = false);
    bool stop(qint64 token);
    bool stopAny();
    qint64 currentToken() const;
    /// <summary>Продлевает владение эксклюзивным обработчиком.</summary>
    /// <param name="token">Токен текущего владельца.</param>
    /// <returns>Признак действующего эксклюзивного владения.</returns>
    bool keepAlive(qint64 token);

private:
    /// <summary>Создаёт контроллер общего обработчика.</summary>
    Xray();
    bool startXray(const QString& cfg);
    bool stopXray();

    qint64 m_currentToken = 0;
    qint64 m_lastToken = 0;
    bool m_exclusive = false;
    QTimer m_ownerTimer;

    static void ctxSockCallback(uintptr_t fd, void* ctx) {
        reinterpret_cast<Xray*>(ctx)->sockCallback(fd);
    }
    static void ctxLogHandler(char* str, void* ctx) {
        reinterpret_cast<Xray*>(ctx)->logHandler(str);
    }

    void sockCallback(uintptr_t fd);
    void logHandler(char* str);

#ifdef Q_OS_LINUX
    QByteArray m_defaultIfaceName;
#else
    int m_defaultIfaceIdx;
#endif

#ifdef Q_OS_MAC
    QString m_uplinkIfaceName;
    QString m_uplinkGateway;
#endif
};

#endif // XRAY_H
