#ifndef XRAY_H
#define XRAY_H

#include <QString>

class Xray
{
public:
    static Xray& getInstance()
    {
        static Xray instance;
        return instance;
    }

    bool startXray(const QString& cfg);
    bool stopXray();

private:
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
};

#endif // XRAY_H
