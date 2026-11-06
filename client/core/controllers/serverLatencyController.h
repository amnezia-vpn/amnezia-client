#ifndef SERVERLATENCYCONTROLLER_H
#define SERVERLATENCYCONTROLLER_H

#include <optional>

#include <QHash>
#include <QObject>

#include "core/models/serverDescription.h"
#include "core/utils/latencyProbe.h"

class ConnectionController;
class ServersController;
class SettingsController;

class ServerLatencyController : public QObject
{
    Q_OBJECT

public:
    explicit ServerLatencyController(ServersController *serversController, SettingsController *settingsController,
                                     ConnectionController *connectionController, QObject *parent = nullptr);

    void testAllServers();
    void testDefaultServer();

    bool isTesting() const;
    bool isTesting(const QString &serverId) const;
    std::optional<LatencyResult> result(const QString &serverId) const;

signals:
    void serverLatencyChanged(const QString &serverId);
    void testingChanged();

private:
    void startProbe(const amnezia::ServerDescription &server);
    std::optional<LatencyEndpoint> endpointFor(const amnezia::ServerDescription &server) const;

    ServersController *m_serversController;
    SettingsController *m_settingsController;
    ConnectionController *m_connectionController;

    QHash<QString, LatencyProbe *> m_activeProbes;
    QHash<QString, LatencyResult> m_results;
};

#endif // SERVERLATENCYCONTROLLER_H
