#include <QDir>
#include <QNetworkInterface>

#include "defines.h"
#include "localserver.h"
#include "log.h"
#include "systemservice.h"
#include "utils.h"

#ifdef Q_OS_WIN
#include "windowsfirewall.h"
#include <memory>

class KillSwitch final {
public:
    explicit KillSwitch(){
        qDebug()<<__FUNCTION__;
        auto cf = WindowsFirewall::instance();
        cf->init();
        //cf->disableKillSwitch();
    }
    ~KillSwitch(){
        qDebug()<<__FUNCTION__;
        auto cf = WindowsFirewall::instance();
        cf->disableKillSwitch();
    }
};
#endif

int runApplication(int argc, char** argv)
{
    QCoreApplication app(argc,argv);
    LocalServer localServer;

    return app.exec();
}


int main(int argc, char **argv)
{
    Utils::initializePath(Utils::systemLogPath());
    Log::initialize();

#ifdef Q_OS_WIN
    KillSwitch ks;
#endif

    if (argc == 2) {
        qInfo() << "Started as console application";
        return runApplication(argc, argv);
    }
    else {
        qInfo() << "Started as system service";
#ifdef Q_OS_WIN
        SystemService systemService(argc, argv);
        return systemService.exec();
#else
        return runApplication(argc, argv);
#endif

    }
}
