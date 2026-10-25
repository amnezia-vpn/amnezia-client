#ifndef LOCALPROXYCONTROLLER_H
#define LOCALPROXYCONTROLLER_H

#include <optional>

#include <QObject>
#include <QString>

class QHttpServer;
class SecureServersRepository;
class SecureAppSettingsRepository;
class SubscriptionController;

class LocalProxyController : public QObject
{
    Q_OBJECT

public:
    LocalProxyController(SecureServersRepository *serversRepository, SecureAppSettingsRepository *appSettingsRepository,
                         SubscriptionController *subscriptionController, QObject *parent = nullptr);
    ~LocalProxyController();

    bool isSupported() const;
    bool isEnabled() const;
    int port() const;
    QString ownerId() const;
    bool isPortUserDefined() const;
    int activePort() const;

    bool setPort(int port);
    bool isPortBusy(int port) const;
    bool enable(const QString &ownerId, int port);
    void disable();

public slots:
    void onServerEdited(const QString &serverId);
    void onServerRemoved(const QString &serverId);
    void onVpnConnectionRequested();

signals:
    void stateChanged();
    void startFailed(const QString &message);
    void stoppedByVpn(const QString &message);

private:
    struct XrayConfig
    {
        QString serializedConfig;
        int proxyPort = 0;
    };

    void start();
    void stop();
    bool syncXray();
    bool startXray();
    bool stopXray();
    bool restartXray();
    void disableWithError(const QString &message);
    void setActivePort(int port);
    int runningProxyPort() const;

    std::optional<XrayConfig> buildXrayConfig() const;
    std::optional<int> selectProxyPort() const;
    std::optional<QString> fetchXrayConfigFromGateway(const QString &serverId) const;

    bool startApi(quint16 port);
    void stopApi();
    bool startXrayEngine(const QString &config);
    bool stopXrayEngine();
    bool isXrayEngineRunning() const;

    SecureServersRepository *m_serversRepository;
    SecureAppSettingsRepository *m_appSettingsRepository;
    SubscriptionController *m_subscriptionController;

    bool m_isEnabled = false;
    QHttpServer *m_httpServer = nullptr;
    quint16 m_apiPort = 0;
    qint64 m_xrayToken = 0;
    quint16 m_appliedProxyPort = 0;
    int m_activePort = 0;
    int m_lastProxyPort = 0;
};

#endif // LOCALPROXYCONTROLLER_H
