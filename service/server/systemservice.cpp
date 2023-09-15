#include "version.h"
#include "localserver.h"
#include "systemservice.h"


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
    setServiceDescription("Service for AmneziaVPN");

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
    m_localServer  = new LocalServer();
}

void SystemService::stop()
{
    delete m_localServer;
}
