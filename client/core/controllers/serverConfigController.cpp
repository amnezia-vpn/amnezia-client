#include "serverConfigController.h"

ServerConfigController::ServerConfigController()
{
}

void ServerConfigController::addServer(const QSharedPointer<ServerConfig> &serverConfig)
{
}

void ServerConfigController::editServer(const QSharedPointer<ServerConfig> &serverConfig, const int serverIndex)
{
}

void ServerConfigController::removeServer(const int serverIndex)
{
}

QSharedPointer<ServerConfig> ServerConfigController::getServer(const int serverIndex)
{
}

QVector<QSharedPointer<ServerConfig> > ServerConfigController::getAllServers()
{
}

void ServerConfigController::setDefaultServerIndex(const int index)
{
}

const int ServerConfigController::getDefaultServerIndex()
{
}
