#ifndef CONFIGURATORBASE_H
#define CONFIGURATORBASE_H

#include <QObject>

class Settings;

#include "containers/containers_defs.h"
#include "core/defs.h"

class ConfiguratorBase : public QObject
{
    Q_OBJECT
public:
    explicit ConfiguratorBase(std::shared_ptr<Settings> settings, QObject *parent = nullptr);

protected:
    std::shared_ptr<Settings> m_settings;
};

#endif // CONFIGURATORBASE_H
