#ifndef SSHCLIENT_H
#define SSHCLIENT_H

#include <QObject>

#include "sshsession.h"

using namespace amnezia;

namespace libssh {
    class Client : public QObject
    {
        Q_OBJECT
    public:
        Client(QObject *parent = nullptr);
        ~Client();

        std::shared_ptr<Session> getSession();
    };
}

#endif // SSHCLIENT_H
