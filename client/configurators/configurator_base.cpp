#include "configurator_base.h"

ConfiguratorBase::ConfiguratorBase(std::shared_ptr<Settings> settings,
    std::shared_ptr<ServerController> serverController, QObject *parent)
    : QObject{parent},
      m_settings(settings),
      m_serverController(serverController)
{

}
