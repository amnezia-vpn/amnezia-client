#include <QCoreApplication>
#include <QProcess>

#include "defines.h"

bool executeProcess(const QString& cmd, const QStringList& args)
{
    QProcess process;
    process.start(cmd, args);
    return process.waitForFinished();
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    executeProcess("sc", QStringList() << "stop" << SERVICE_NAME);
    executeProcess("sc", QStringList() << "delete" << SERVICE_NAME);

    return 0;
}
