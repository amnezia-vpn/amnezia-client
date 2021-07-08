#include <QDir>

#include "defines.h"
#include "localserver.h"
#include "log.h"
#include "systemservice.h"
#include "utils.h"

int runApplication(int argc, char** argv)
{
    QCoreApplication app(argc,argv);
    LocalServer localServer;
//    if (!localServer.isRunning()) {
//        return -1;
//    }
    return app.exec();
}
int main(int argc, char **argv)
{
    Utils::initializePath(Utils::systemLogPath());

    Log::initialize();

    if (argc == 2) {
        qInfo() << "Started as console application";
        return runApplication(argc, argv);
    } else {
        qInfo() << "Started as system service";
#ifdef Q_OS_WIN
        SystemService systemService(argc, argv);
        return systemService.exec();

#else
        //daemon(0,0);
        return runApplication(argc, argv);
#endif

    }

    // Never reached
    return 0;
}
