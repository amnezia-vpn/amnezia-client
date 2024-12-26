#ifndef SETTINGSCONTROLLER_H
#define SETTINGSCONTROLLER_H

#include <QObject>

#include "ui/models/containers_model.h"
#include "ui/models/languageModel.h"
#include "ui/models/servers_model.h"
#include "ui/models/sites_model.h"
#include "ui/models/appSplitTunnelingModel.h"

class SettingsController : public QObject
{
    Q_OBJECT
public:
    explicit SettingsController(const QSharedPointer<ServersModel> &serversModel,
                                const QSharedPointer<ContainersModel> &containersModel,
                                const QSharedPointer<LanguageModel> &languageModel,
                                const QSharedPointer<SitesModel> &sitesModel,
                                const QSharedPointer<AppSplitTunnelingModel> &appSplitTunnelingModel,
                                const std::shared_ptr<Settings> &settings, QObject *parent = nullptr);

    Q_PROPERTY(QString primaryDns READ getPrimaryDns WRITE setPrimaryDns NOTIFY primaryDnsChanged)
    Q_PROPERTY(QString secondaryDns READ getSecondaryDns WRITE setSecondaryDns NOTIFY secondaryDnsChanged)
    Q_PROPERTY(bool isLoggingEnabled READ isLoggingEnabled WRITE toggleLogging NOTIFY loggingStateChanged)
    Q_PROPERTY(bool isNotificationPermissionGranted READ isNotificationPermissionGranted NOTIFY onNotificationStateChanged)

    Q_PROPERTY(bool isDevModeEnabled READ isDevModeEnabled NOTIFY devModeEnabled)
    Q_PROPERTY(QString gatewayEndpoint READ getGatewayEndpoint WRITE setGatewayEndpoint NOTIFY gatewayEndpointChanged)
    Q_PROPERTY(bool isDevGatewayEnv READ isDevGatewayEnv WRITE toggleDevGatewayEnv NOTIFY devGatewayEnvChanged)

    Q_PROPERTY(bool isHomeAdLabelVisible READ isHomeAdLabelVisible NOTIFY isHomeAdLabelVisibleChanged)

public slots:
    void toggleAmneziaDns(bool enable);
    bool isAmneziaDnsEnabled();

    QString getPrimaryDns();
    void setPrimaryDns(const QString &dns);

    QString getSecondaryDns();
    void setSecondaryDns(const QString &dns);

    bool isLoggingEnabled();
    void toggleLogging(bool enable);

    void openLogsFolder();
    void openServiceLogsFolder();
    void exportLogsFile(const QString &fileName);
    void exportServiceLogsFile(const QString &fileName);
    void clearLogs();

    void backupAppConfig(const QString &fileName);
    void restoreAppConfig(const QString &fileName);
    void restoreAppConfigFromData(const QByteArray &data);

    QString getAppVersion();

    void clearSettings();

    bool isAutoConnectEnabled();
    void toggleAutoConnect(bool enable);

    bool isAutoStartEnabled();
    void toggleAutoStart(bool enable);

    bool isStartMinimizedEnabled();
    void toggleStartMinimized(bool enable);

    bool isScreenshotsEnabled();
    void toggleScreenshotsEnabled(bool enable);

    bool isCameraPresent();

    bool isKillSwitchEnabled();
    void toggleKillSwitch(bool enable);

    bool isNotificationPermissionGranted();
    void requestNotificationPermission();

    QString getInstallationUuid();

    void enableDevMode();
    bool isDevModeEnabled();

    void resetGatewayEndpoint();
    void setGatewayEndpoint(const QString &endpoint);
    QString getGatewayEndpoint();
    bool isDevGatewayEnv();
    void toggleDevGatewayEnv(bool enabled);

    bool isOnTv();

    bool isHomeAdLabelVisible();
    void disableHomeAdLabel();

signals:
    void primaryDnsChanged();
    void secondaryDnsChanged();
    void loggingStateChanged();

    void restoreBackupFinished();
    void changeSettingsFinished(const QString &finishedMessage);
    void changeSettingsErrorOccurred(const QString &errorMessage);

    void saveFile(const QString &fileName, const QString &data);

    void importBackupFromOutside(QString filePath);

    void amneziaDnsToggled(bool enable);

    void loggingDisableByWatcher();

    void onNotificationStateChanged();

    void devModeEnabled();
    void gatewayEndpointChanged(const QString &endpoint);
    void devGatewayEnvChanged(bool enabled);

    void isHomeAdLabelVisibleChanged(bool visible);

private:
    QSharedPointer<ServersModel> m_serversModel;
    QSharedPointer<ContainersModel> m_containersModel;
    QSharedPointer<LanguageModel> m_languageModel;
    QSharedPointer<SitesModel> m_sitesModel;
    QSharedPointer<AppSplitTunnelingModel> m_appSplitTunnelingModel;
    std::shared_ptr<Settings> m_settings;

    QString m_appVersion;

    QDateTime m_loggingDisableDate;

    bool m_isDevModeEnabled = false;

    void checkIfNeedDisableLogs();
};

#endif // SETTINGSCONTROLLER_H
