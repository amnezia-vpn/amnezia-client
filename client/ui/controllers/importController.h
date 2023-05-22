#ifndef IMPORTCONTROLLER_H
#define IMPORTCONTROLLER_H

#include <QObject>

#include "core/defs.h"
#include "containers/containers_defs.h"
#include "ui/models/servers_model.h"
#include "ui/models/containers_model.h"

class ImportController : public QObject
{
    Q_OBJECT
public:
    explicit ImportController(const QSharedPointer<ServersModel> &serversModel,
                              const QSharedPointer<ContainersModel> &containersModel,
                              const std::shared_ptr<Settings> &settings,
                              QObject *parent = nullptr);

public slots:
    bool importFromFile(const QUrl &fileUrl);

signals:
    void importFinished();
private:
    bool import(const QJsonObject &config);
    bool importAmneziaConfig(QString data);
    bool importOpenVpnConfig(const QString &data);
    bool importWireGuardConfig(const QString &data);

    QSharedPointer<ServersModel> m_serversModel;
    QSharedPointer<ContainersModel> m_containersModel;
    std::shared_ptr<Settings> m_settings;

};

#endif // IMPORTCONTROLLER_H
