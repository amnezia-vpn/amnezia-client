#ifndef SETTINGSCONFIGCONTROLLER_H
#define SETTINGSCONFIGCONTROLLER_H

#include <QObject>
#include <QFuture>

#include "core/defs.h"

class Settings;

using namespace amnezia;

class SettingsConfigController : public QObject
{
    Q_OBJECT

public:
    explicit SettingsConfigController(std::shared_ptr<Settings> settings, QObject *parent = nullptr);

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

#endif // SETTINGSCONFIGCONTROLLER_H 
