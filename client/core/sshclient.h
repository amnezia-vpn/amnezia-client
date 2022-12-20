#ifndef SSHCLIENT_H
#define SSHCLIENT_H

#include <QObject>

#include "sshsession.h"

using namespace amnezia;

class SshClient : public QObject
{
    Q_OBJECT
public:
    SshClient(QObject *parent = nullptr);
    ~SshClient();

    std::shared_ptr<SshSession> getSession();
};

#endif // SSHCLIENT_H
