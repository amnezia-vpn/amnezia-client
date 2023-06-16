#ifndef SETTINGSCONTROLLER_H
#define SETTINGSCONTROLLER_H

#include <QObject>

#include "ui/models/containers_model.h"
#include "ui/models/servers_model.h"

class SettingsController : public QObject
{
    Q_OBJECT
public:
    explicit SettingsController(const QSharedPointer<ServersModel> &serversModel,
                                const QSharedPointer<ContainersModel> &containersModel,
                                const std::shared_ptr<Settings> &settings,
                                QObject *parent = nullptr);

    Q_PROPERTY(QString primaryDns READ getPrimaryDns WRITE setPrimaryDns NOTIFY primaryDnsChanged)
    Q_PROPERTY(
        QString secondaryDns READ getSecondaryDns WRITE setSecondaryDns NOTIFY secondaryDnsChanged)

public slots:
    void setAmneziaDns(bool enable);
    bool isAmneziaDnsEnabled();

    QString getPrimaryDns();
    void setPrimaryDns(const QString &dns);

    QString getSecondaryDns();
    void setSecondaryDns(const QString &dns);

    bool isSaveLogsEnabled();
    void setSaveLogs(bool enable);

    void openLogsFolder();
    void exportLogsFile();
    void clearLogs();

    void backupAppConfig();
    void restoreAppConfig();

    QString getAppVersion();

signals:
    void primaryDnsChanged();
    void secondaryDnsChanged();

private:
    QSharedPointer<ServersModel> m_serversModel;
    QSharedPointer<ContainersModel> m_containersModel;
    std::shared_ptr<Settings> m_settings;

    QString m_appVersion;
};

#endif // SETTINGSCONTROLLER_H
