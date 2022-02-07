#ifndef IPCSERVERPROCESS_H
#define IPCSERVERPROCESS_H

#include <QObject>

#ifndef Q_OS_IOS
#include "rep_ipc_process_interface_source.h"

class IpcServerProcess : public IpcProcessInterfaceSource
{
    Q_OBJECT
public:
    explicit IpcServerProcess(QObject *parent = nullptr);
    virtual ~IpcServerProcess();

    void start(const QString &program, const QStringList &arguments) override;
    void start() override;
    void close() override;

    void setArguments(const QStringList &arguments) override;
    void setInputChannelMode(QProcess::InputChannelMode mode) override;
    void setNativeArguments(const QString &arguments) override;
    void setProcessChannelMode(QProcess::ProcessChannelMode mode) override;
    void setProgram(const QString &program) override;
    void setWorkingDirectory(const QString &dir) override;

    QByteArray readAll() override;
    QByteArray readAllStandardError() override;
    QByteArray readAllStandardOutput() override;

signals:

private:
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
