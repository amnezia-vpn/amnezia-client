#include "sshclient.h"

#include <libssh/libssh.h>
#include <libssh/sftp.h>

SshClient::SshClient(QObject *parent) : QObject(parent)
{
    ssh_init();
}

SshClient::~SshClient()
{
    ssh_finalize();
}

std::shared_ptr<SshSession> SshClient::getSession()
{
    return std::make_shared<SshSession>();
}
