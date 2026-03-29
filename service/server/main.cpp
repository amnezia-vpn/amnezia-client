#include <QDir>

#include "version.h"
#include "localserver.h"
#include "logger.h"
#include "systemservice.h"
#include "utilities.h"

#ifdef Q_OS_WIN
#include "platforms/windows/daemon/windowsdaemontunnel.h"

namespace {
int s_argc = 0;
char** s_argv = nullptr;
}  // namespace

#endif

int runApplication(int argc, char** argv)
{
    QCoreApplication app(argc,argv);

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

    LocalServer localServer;
    return app.exec();

}


int main(int argc, char **argv)
{
    Utils::initializePath(Logger::systemLogDir());

    if (argc >= 2) {
#ifdef Q_OS_WIN
        const QString firstArg = QString::fromLocal8Bit(argv[1]);

        // Keep the low-level tunnel daemon path as a plain console process,
        // but let all QtService control flags (-i/-u/-e/...) reach QtService.
        if (firstArg == "tunneldaemon") {
            qInfo() << "Started as console application";
            return runApplication(argc, argv);
        }

        qInfo() << "Started as service controller";
        SystemService systemService(argc, argv);
        return systemService.exec();
#else
        qInfo() << "Started as console application";
        return runApplication(argc, argv);
#endif
    }

    qInfo() << "Started as system service";
#ifdef Q_OS_WIN
    SystemService systemService(argc, argv);
    return systemService.exec();
#else
    return runApplication(argc, argv);
#endif
}
