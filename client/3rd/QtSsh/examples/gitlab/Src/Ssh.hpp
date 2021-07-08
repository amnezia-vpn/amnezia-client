#pragma once

#include <memory>
#include <cstdlib>
#include <functional>

#include <QObject>
#include <QtSsh/sshconnection.h>
#include <QtSsh/sshremoteprocess.h>

using namespace QSsh;

class Ssh : public QObject {
    Q_OBJECT
public:
    explicit Ssh(QObject *parent = nullptr);

    Q_INVOKABLE void connectToHost();
    void create();

private:
    SshConnectionParameters mParams;
    std::shared_ptr<SshConnection> mConnections;
    QSharedPointer<SshRemoteProcess> mRemoteProcess;
};

