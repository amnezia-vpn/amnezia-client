#include "version.h"
#include "localserver.h"
#include "systemservice.h"
#include <QTimer>


#ifdef Q_OS_WIN
#include "platforms/windows/daemon/windowsdaemontunnel.h"

namespace {
int s_argc = 0;
char** s_argv = nullptr;
}  // namespace
#endif

SystemService::SystemService(int argc, char **argv)
    : QtService<QCoreApplication>(argc, argv, SERVICE_NAME)
{
    setServiceDescription("FBLink VPN Service");

#ifdef Q_OS_WIN
    if(argc > 2){
        s_argc = argc;
        s_argv = argv;
        QStringList tokens;

        for (int i = 1; i < argc; ++i) {
            tokens.append(QString(argv[i]));
        }

        if (!tokens.empty() && tokens[0] == "tunneldaemon") {
            WindowsDaemonTunnel *daemon = new WindowsDaemonTunnel();
            daemon->run(tokens);
        }

    }
#endif

}

void SystemService::start()
{
    QCoreApplication* app = application();
    if (!app) {
        app = QCoreApplication::instance();
    }

    // Return control to SCM quickly; heavy init is deferred to event loop.
    QTimer::singleShot(0, app, [this]() {
        if (!m_localServer) {
            m_localServer = new LocalServer();
        }
    });
}

void SystemService::stop()
{
    delete m_localServer;
    m_localServer = nullptr;
}
