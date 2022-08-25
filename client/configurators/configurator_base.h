#ifndef CONFIGURATORBASE_H
#define CONFIGURATORBASE_H

#include <QObject>

class Settings;
class ServerController;

#include "containers/containers_defs.h"
#include "core/defs.h"

class ConfiguratorBase : public QObject
{
    Q_OBJECT
public:
    explicit ConfiguratorBase(std::shared_ptr<Settings> settings,
        std::shared_ptr<ServerController> serverController, QObject *parent = nullptr);

protected:
    std::shared_ptr<Settings> m_settings;
    std::shared_ptr<ServerController> m_serverController;

};

#endif // CONFIGURATORBASE_H
