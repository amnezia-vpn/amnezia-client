#ifndef HYSTERIA2_SERVICE_H
#define HYSTERIA2_SERVICE_H

#include <QString>

// Service-side Hysteria2 handler.
// Wraps the amnezia_hysteria2 C library, which is compiled from the
// Hysteria2 Go client (see client/3rd/hysteria2-desktop/).
//
// The library is pre-built and stored in:
//   client/3rd-prebuilt/3rd-prebuilt/amnezia_hysteria2/{platform}/
//
// Build the library:
//   cd client/3rd/hysteria2-desktop && ./build.sh
class Hysteria2Service
{
public:
    static Hysteria2Service& getInstance()
    {
        static Hysteria2Service instance;
        return instance;
    }

    // Start a Hysteria2 client with the provided YAML config string.
    // The SOCKS5 proxy will be available at 127.0.0.1:10808 after a
    // successful call.
    bool start(const QString& configYaml);

    // Stop the running Hysteria2 client.
    bool stop();

private:
    static void ctxSockCallback(uintptr_t fd, void* ctx) {
        reinterpret_cast<Hysteria2Service*>(ctx)->sockCallback(fd);
    }
    static void ctxLogHandler(char* str, void* ctx) {
        reinterpret_cast<Hysteria2Service*>(ctx)->logHandler(str);
    }

    void sockCallback(uintptr_t fd);
    void logHandler(char* str);

#ifdef Q_OS_LINUX
    QByteArray m_defaultIfaceName;
#else
    int m_defaultIfaceIdx = 0;
#endif
};

#endif // HYSTERIA2_SERVICE_H
