#ifndef SETTINGSCONTROLLER_H
#define SETTINGSCONTROLLER_H

#include <QObject>
#include <QJsonObject>
#include <QByteArray>
#include <QDateTime>

#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"
#include "core/repositories/secureServersRepository.h"
#include "core/repositories/secureAppSettingsRepository.h"

using namespace amnezia;

class SettingsController : public QObject
{
    Q_OBJECT
public:
    explicit SettingsController(SecureServersRepository* serversRepository,
                               SecureAppSettingsRepository* appSettingsRepository,
                               QObject* parent = nullptr);
    ~SettingsController() = default;

    void toggleAmneziaDns(bool enable);
    bool isAmneziaDnsEnabled() const;

    QString getPrimaryDns() const;
    void setPrimaryDns(const QString &dns);

    QString getSecondaryDns() const;
    void setSecondaryDns(const QString &dns);

    bool isLoggingEnabled() const;
    void toggleLogging(bool enable);

    void clearLogs();

    QByteArray backupAppConfig() const;
    ErrorCode restoreAppConfigFromData(const QByteArray &data);

    QString getAppVersion() const;

    void clearSettings();

    bool isAutoConnectEnabled() const;
    void toggleAutoConnect(bool enable);

    bool isAutoStartEnabled() const;
    void toggleAutoStart(bool enable);

    bool isStartMinimizedEnabled() const;
    void toggleStartMinimized(bool enable);

    bool isScreenshotsEnabled() const;
    void toggleScreenshotsEnabled(bool enable);

    bool isNewsNotificationsEnabled() const;
    void toggleNewsNotificationsEnabled(bool enable);

    bool isKillSwitchEnabled() const;
    void toggleKillSwitch(bool enable);

    bool isStrictKillSwitchEnabled() const;
    void toggleStrictKillSwitch(bool enable);

    QString getInstallationUuid(bool createIfNotExists = true) const;

    void enableDevMode();
    
    bool isPremV1MigrationReminderActive() const;
    void disablePremV1MigrationReminder();
    
    QString nextAvailableServerName() const;
    bool isDevModeEnabled() const;

    void resetGatewayEndpoint();
    void setGatewayEndpoint(const QString &endpoint);
    QString getGatewayEndpoint() const;
    bool isDevGatewayEnv() const;
    void toggleDevGatewayEnv(bool enabled);

    bool isHomeAdLabelVisible() const;
    void disableHomeAdLabel();

    void checkIfNeedDisableLogs();

    QLocale getAppLanguage() const;
    void setAppLanguage(const QLocale &locale);

signals:
    void siteSplitTunnelingRouteModeChanged(RouteMode mode);
    void siteSplitTunnelingToggled(bool enabled);
    void appSplitTunnelingRouteModeChanged(AppsRouteMode mode);
    void appSplitTunnelingToggled(bool enabled);
    void appSplitTunnelingClearAppsList();

private:
    QString getPlatform() const;

    SecureServersRepository* m_serversRepository;
    SecureAppSettingsRepository* m_appSettingsRepository;

    QString m_appVersion;
    QDateTime m_loggingDisableDate;
    bool m_isDevModeEnabled = false;
};

#endif


