#ifndef SSH_CONFIGURATOR_H
#define SSH_CONFIGURATOR_H

#include <QObject>
#include <QProcessEnvironment>

#include "configurator_base.h"
#include "core/defs.h"

class SshConfigurator : ConfiguratorBase
{
    Q_OBJECT
public:
    SshConfigurator(std::shared_ptr<Settings> settings,
        std::shared_ptr<ServerController> serverController, QObject *parent = nullptr);

    QProcessEnvironment prepareEnv();
    QString convertOpenSShKey(const QString &key);
    void openSshTerminal(const ServerCredentials &credentials);

};

#endif // SSH_CONFIGURATOR_H
