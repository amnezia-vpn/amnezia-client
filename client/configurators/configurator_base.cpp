#include "configurator_base.h"

ConfiguratorBase::ConfiguratorBase(std::shared_ptr<Settings> settings, QObject *parent)
    : QObject{parent},
      m_settings(settings)
{

}
