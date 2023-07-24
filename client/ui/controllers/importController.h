#ifndef IMPORTCONTROLLER_H
#define IMPORTCONTROLLER_H

#include <QObject>

#include "containers/containers_defs.h"
#include "core/defs.h"
#include "ui/models/containers_model.h"
#include "ui/models/servers_model.h"

class ImportController : public QObject
{
    Q_OBJECT
public:
    explicit ImportController(const QSharedPointer<ServersModel> &serversModel,
                              const QSharedPointer<ContainersModel> &containersModel,
                              const std::shared_ptr<Settings> &settings, QObject *parent = nullptr);

public slots:
    void importConfig();
    void extractConfigFromFile();
    void extractConfigFromData(QString &data);
    void extractConfigFromCode(QString code);
    void extractConfigFromQr();
    QString getConfig();
    QString getConfigFileName();

signals:
    void importFinished();
    void importErrorOccurred(QString errorMessage);

private:
    QJsonObject extractAmneziaConfig(QString &data);
    QJsonObject extractOpenVpnConfig(const QString &data);
    QJsonObject extractWireGuardConfig(const QString &data);

    QSharedPointer<ServersModel> m_serversModel;
    QSharedPointer<ContainersModel> m_containersModel;
    std::shared_ptr<Settings> m_settings;

    QJsonObject m_config;
    QString m_configFileName;
};

#endif // IMPORTCONTROLLER_H
