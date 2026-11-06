#ifndef SERVERLATENCYUICONTROLLER_H
#define SERVERLATENCYUICONTROLLER_H

#include <QObject>

#include "core/controllers/serverLatencyController.h"

class ServersController;
class ServersModel;

class ServerLatencyUiController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool isTesting READ isTesting NOTIFY testingChanged)
    Q_PROPERTY(int defaultServerLatencyMs READ defaultServerLatencyMs NOTIFY defaultServerLatencyChanged)
    Q_PROPERTY(int defaultServerQuality READ defaultServerQuality NOTIFY defaultServerLatencyChanged)
    Q_PROPERTY(bool isDefaultServerTesting READ isDefaultServerTesting NOTIFY defaultServerLatencyChanged)

public:
    explicit ServerLatencyUiController(ServerLatencyController *latencyController, ServersController *serversController,
                                       ServersModel *serversModel, QObject *parent = nullptr);

    static void declareQmlServerQualityEnum();

    bool isTesting() const;
    int defaultServerLatencyMs() const;
    int defaultServerQuality() const;
    bool isDefaultServerTesting() const;

public slots:
    void testAllServers();
    void testDefaultServer();
    void onDefaultServerChanged();

signals:
    void testingChanged();
    void defaultServerLatencyChanged();

private:
    void onServerLatencyChanged(const QString &serverId);
    LatencyResult defaultServerResult() const;

    ServerLatencyController *m_latencyController;
    ServersController *m_serversController;
    ServersModel *m_serversModel;
};

#endif // SERVERLATENCYUICONTROLLER_H
