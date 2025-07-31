#ifndef SETTINGSCONTROLLER_H
#define SETTINGSCONTROLLER_H

#include <QObject>
#include <QFuture>
#include <QDateTime>

#include "core/defs.h"

class Settings;

using namespace amnezia;

class SettingsController : public QObject
{
    Q_OBJECT

public:
    explicit SettingsController(std::shared_ptr<Settings> settings, QObject *parent = nullptr);

    void resetAllSettings();
    
    void configureDns(const QString &primaryDns, const QString &secondaryDns);
    void toggleAmneziaDns(bool enable);
    
    void configureLogging(bool enable);
    void checkLoggingExpiration();
    void clearLogs();
    
    void configureKillSwitch(bool enable, bool strict = false);
    
    void configureAutoStart(bool enable);
    void configureAutoConnect(bool enable);
    void configureStartMinimized(bool enable);
    void configureScreenshots(bool enable);

    QString getPrimaryDns() const;
    QString getSecondaryDns() const;
    bool isAmneziaDnsEnabled() const;
    bool isLoggingEnabled() const;
    bool isKillSwitchEnabled() const;
    bool isStrictKillSwitchEnabled() const;
    bool isAutoStartEnabled() const;
    bool isAutoConnectEnabled() const;
    bool isStartMinimizedEnabled() const;
    bool isScreenshotsEnabled() const;

    // Backup/restore functionality
    QByteArray backupAppConfig() const;
    bool restoreAppConfig(const QByteArray &data);

    // Installation UUID
    QString getInstallationUuid() const;

    // Gateway endpoint functionality
    void resetGatewayEndpoint();
    void setGatewayEndpoint(const QString &endpoint);
    QString getGatewayEndpoint() const;
    bool isDevGatewayEnv() const;
    void toggleDevGatewayEnv(bool enabled);
    void setDevGatewayEndpoint();

    // Home ad label
    bool isHomeAdLabelVisible() const;
    void disableHomeAdLabel();

    // Log date
    QDateTime getLogEnableDate() const;

    QLocale getAppLanguage() const;
    void setAppLanguage(const QLocale &locale);

signals:
    void settingsReset();
    void dnsConfigChanged();
    void loggingConfigChanged();
    void killSwitchConfigChanged();
    void autoStartConfigChanged();
    void loggingExpired();

private:
    std::shared_ptr<Settings> m_settings;
};

#endif // SETTINGSCONTROLLER_H 
