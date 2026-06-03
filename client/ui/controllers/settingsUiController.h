#ifndef SETTINGSUICONTROLLER_H
#define SETTINGSUICONTROLLER_H

#include <QObject>

#include "core/controllers/settingsController.h"
#include "core/controllers/serversController.h"
#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"

class SettingsUiController : public QObject
{
    Q_OBJECT
public:
    explicit SettingsUiController(SettingsController* settingsController,
                                 ServersController* serversController,
                                 QObject *parent = nullptr);

    Q_PROPERTY(QString primaryDns READ getPrimaryDns WRITE setPrimaryDns NOTIFY primaryDnsChanged)
    Q_PROPERTY(QString secondaryDns READ getSecondaryDns WRITE setSecondaryDns NOTIFY secondaryDnsChanged)
    Q_PROPERTY(bool isLoggingEnabled READ isLoggingEnabled WRITE toggleLogging NOTIFY loggingStateChanged)
    Q_PROPERTY(bool isNotificationPermissionGranted READ isNotificationPermissionGranted NOTIFY onNotificationStateChanged)
    Q_PROPERTY(bool isKillSwitchEnabled READ isKillSwitchEnabled WRITE toggleKillSwitch NOTIFY killSwitchEnabledChanged)
    Q_PROPERTY(bool strictKillSwitchEnabled READ isStrictKillSwitchEnabled WRITE toggleStrictKillSwitch NOTIFY strictKillSwitchEnabledChanged)

    Q_PROPERTY(bool isDevModeEnabled READ isDevModeEnabled NOTIFY devModeEnabled)
    Q_PROPERTY(QString gatewayEndpoint READ getGatewayEndpoint WRITE setGatewayEndpoint NOTIFY gatewayEndpointChanged)
    Q_PROPERTY(bool isDevGatewayEnv READ isDevGatewayEnv WRITE toggleDevGatewayEnv NOTIFY devGatewayEnvChanged)

    Q_PROPERTY(bool isHomeAdLabelVisible READ isHomeAdLabelVisible NOTIFY isHomeAdLabelVisibleChanged)
    Q_PROPERTY(bool autoStartEnabled READ isAutoStartEnabled NOTIFY autoStartChanged)
    Q_PROPERTY(bool startMinimized READ isStartMinimizedEnabled NOTIFY startMinimizedChanged)

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

    bool isNewsNotificationsEnabled();
    void toggleNewsNotificationsEnabled(bool enable);

    bool isScreenshotsEnabled();
    void toggleScreenshotsEnabled(bool enable);

    bool isCameraPresent();

    bool isKillSwitchEnabled();
    void toggleKillSwitch(bool enable);

    bool isStrictKillSwitchEnabled();
    void toggleStrictKillSwitch(bool enable);

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
    void killSwitchEnabledChanged();
    void strictKillSwitchEnabledChanged(bool enabled);

    void restoreBackupFinished();
    void changeSettingsFinished(const QString &finishedMessage);
    void errorOccurred(ErrorCode errorCode);

    void saveFile(const QString &fileName, const QString &data);

    void importBackupFromOutside(QString filePath);

    void amneziaDnsToggled(bool enable);

    void loggingDisableByWatcher();

    void appLanguageChanged();
    void resetLanguageToSystem();

    void onNotificationStateChanged();

    void devModeEnabled();
    void gatewayEndpointChanged(const QString &endpoint);
    void devGatewayEnvChanged(bool enabled);

    void activityPaused();
    void activityResumed();

    void isHomeAdLabelVisibleChanged(bool visible);
    void autoStartChanged();
    void startMinimizedChanged();

private:
    SettingsController* m_settingsController;
    ServersController* m_serversController;
};

#endif
