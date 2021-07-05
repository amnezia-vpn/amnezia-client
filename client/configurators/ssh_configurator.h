#ifndef SSH_CONFIGURATOR_H
#define SSH_CONFIGURATOR_H

#include <QObject>
#include <QProcessEnvironment>

#include "core/defs.h"
#include "settings.h"
#include "core/servercontroller.h"

class SshConfigurator
{
public:
    static QProcessEnvironment prepareEnv();
    static QString convertOpenSShKey(const QString &key);
    static void openSshTerminal(const ServerCredentials &credentials);

};

#endif // SSH_CONFIGURATOR_H
