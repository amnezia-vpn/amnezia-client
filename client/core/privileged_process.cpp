#include "privileged_process.h"

#ifndef Q_OS_IOS
PrivilegedProcess::PrivilegedProcess() :
    IpcProcessInterfaceReplica()
{
}

PrivilegedProcess::~PrivilegedProcess()
{
    qDebug() << "PrivilegedProcess::~PrivilegedProcess()";
}

void PrivilegedProcess::waitForFinished(int msecs)
{
    QSharedPointer<QEventLoop> loop(new QEventLoop);
    connect(this, &PrivilegedProcess::finished, this, [this, loop](int exitCode, QProcess::ExitStatus exitStatus) mutable{
        loop->quit();
        loop.clear();
    });

    QTimer::singleShot(msecs, this, [this, loop]() mutable {
        loop->quit();
        loop.clear();
    });

    loop->exec();
}
#endif // Q_OS_IOS
