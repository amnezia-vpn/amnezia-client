#ifndef IPCSERVERPROCESS_H
#define IPCSERVERPROCESS_H

#include "ipc.h"
#include <QObject>

#ifndef Q_OS_IOS
#include "rep_ipc_process_interface_source.h"

class IpcServerProcess : public IpcProcessInterfaceSource
{
    Q_OBJECT
public:
    explicit IpcServerProcess(QObject *parent = nullptr);
    virtual ~IpcServerProcess();

    void start() override;
    void terminate() override;
    void kill() override;
    void close() override;

    void setArguments(const QStringList &arguments) override;
    void setInputChannelMode(QProcess::InputChannelMode mode) override;
    void setNativeArguments(const QString &arguments) override;
    void setProcessChannelMode(QProcess::ProcessChannelMode mode) override;
    void setProgram(int programId) override;
    void setWorkingDirectory(const QString &dir) override;

    QByteArray readAll() override;
    QByteArray readAllStandardError() override;
    QByteArray readAllStandardOutput() override;

    bool waitForStarted() override;
    bool waitForStarted(int msecs) override;
    bool waitForFinished() override;
    bool waitForFinished(int msecs) override;

signals:

private:
    amnezia::PermittedProcess m_program = amnezia::PermittedProcess::Invalid;
    QSharedPointer<QProcess> m_process;
};

#else
class IpcServerProcess : public QObject
{
    Q_OBJECT

public:
    explicit IpcServerProcess(QObject *parent = nullptr);
};
#endif

#endif // IPCSERVERPROCESS_H
