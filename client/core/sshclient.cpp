#include "sshclient.h"

namespace libssh {
    Client::Client(QObject *parent) : QObject(parent)
    {
        ssh_init();
    }

    Client::~Client()
    {
        ssh_finalize();
    }

    std::shared_ptr<Session> Client::getSession()
    {
        return std::make_shared<Session>();
    }
}
