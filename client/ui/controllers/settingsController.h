#ifndef SETTINGSCONTROLLER_H
#define SETTINGSCONTROLLER_H

#include <QObject>

#include "ui/models/containers_model.h"
#include "ui/controllers/languageUiController.h"
#include "ui/models/languageModel.h"
#include "ui/models/servers_model.h"
#include "core/controllers/sitesController.h"
#include "core/controllers/appSplitTunnelingController.h"
#include "core/repositories/qServersRepository.h"
#include "core/repositories/qAppSettingsRepository.h"

class SettingsController : public QObject
{
    Q_OBJECT
public:
    explicit SettingsController(ServersModel* serversModel,
                                ContainersModel* containersModel,
                                LanguageUiController* languageUiController,
                                SitesController* sitesController,
                                AppSplitTunnelingController* appSplitTunnelingController,
                                QServersRepository* serversRepository,
                                QAppSettingsRepository* appSettingsRepository,
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
    Q_PROPERTY(bool startMinimized READ isStartMinimizedEnabled NOTIFY startMinimizedChanged)
    Q_PROPERTY(int safeAreaTopMargin READ getSafeAreaTopMargin NOTIFY safeAreaTopMarginChanged)
    Q_PROPERTY(int safeAreaBottomMargin READ getSafeAreaBottomMargin NOTIFY safeAreaBottomMarginChanged)
    Q_PROPERTY(int imeHeight READ getImeHeight NOTIFY imeHeightChanged)

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
    bool isEdgeToEdgeEnabled();
    int getStatusBarHeight();
    int getNavigationBarHeight();
    int getSafeAreaTopMargin();
    int getSafeAreaBottomMargin();
    int getImeHeight();

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
    void changeSettingsErrorOccurred(const QString &errorMessage);

    void saveFile(const QString &fileName, const QString &data);

    void importBackupFromOutside(QString filePath);

    void amneziaDnsToggled(bool enable);

    void loggingDisableByWatcher();

    void appLanguageChanged(const LanguageSettings::AvailableLanguageEnum language);
    void resetLanguageToSystem();

    void onNotificationStateChanged();

    void devModeEnabled();
    void gatewayEndpointChanged(const QString &endpoint);
    void devGatewayEnvChanged(bool enabled);
    
    void imeHeightChanged(int height);
    void safeAreaTopMarginChanged();
    void safeAreaBottomMarginChanged();

    void isHomeAdLabelVisibleChanged(bool visible);
    void startMinimizedChanged();

private:
    ServersModel* m_serversModel;
    ContainersModel* m_containersModel;
    LanguageUiController* m_languageUiController;
    SitesController* m_sitesController;
    AppSplitTunnelingController* m_appSplitTunnelingController;
    QServersRepository* m_serversRepository;
    QAppSettingsRepository* m_appSettingsRepository;
    
    mutable int m_cachedStatusBarHeight = -1;
    mutable int m_cachedNavigationBarHeight = -1;
    mutable bool m_cachedEdgeToEdgeEnabled = false;
    mutable bool m_edgeToEdgeCached = false;
    int m_imeHeight = 0;

    QString m_appVersion;

    QString getPlatform();

    QDateTime m_loggingDisableDate;

    bool m_isDevModeEnabled = false;

    void checkIfNeedDisableLogs();
};

#endif // SETTINGSCONTROLLER_H
