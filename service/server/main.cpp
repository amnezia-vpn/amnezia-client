#include <QSettings>
#include <QDir>

#include "systemservice.h"
#include "log.h"
#include "defines.h"
#include "localserver.h"

int main(int argc, char **argv)
{
#if !defined(Q_OS_WIN)
    // QtService stores service settings in SystemScope, which normally require root privileges.
    // To allow testing this example as non-root, we change the directory of the SystemScope settings file.
    QSettings::setPath(QSettings::NativeFormat, QSettings::SystemScope, QDir::tempPath());
    qWarning("(Example uses dummy settings file: %s/QtSoftware.conf)", QDir::tempPath().toLatin1().constData());
#endif

    Log::initialize();

    if (argc == 2) {
        qInfo() << "Started as console application";
        QCoreApplication app(argc,argv);
        LocalServer localServer(SERVICE_NAME);
        if (!localServer.isRunning()) {
            return -1;
        }
        return app.exec();
    } else {
        qInfo() << "Started as system service";
        SystemService systemService(argc, argv);
        return systemService.exec();
    }
}
