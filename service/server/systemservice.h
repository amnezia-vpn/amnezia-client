#ifndef SYSTEMSERVICE_H
#define SYSTEMSERVICE_H

#include <QCoreApplication>

#include "qtservice.h"

class LocalServer;

class SystemService : public QtService<QCoreApplication>
{

public:
    SystemService(int argc, char** argv);

protected:
    void start() override;
    void stop() override;

private:
    LocalServer* m_localServer;
};

#endif // SYSTEMSERVICE_H
