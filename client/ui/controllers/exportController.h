#ifndef EXPORTCONTROLLER_H
#define EXPORTCONTROLLER_H

#include <QObject>

#include "core/controllers/serversController.h"
#include "core/controllers/clientManagementController.h"
#include "core/repositories/qAppSettingsRepository.h"
#include "ui/models/containers_model.h"
#include "ui/models/servers_model.h"

class ExportController : public QObject
{
    Q_OBJECT
public:
    explicit ExportController(ServersController* serversController,
                              ServersModel* serversModel,
                              ContainersModel* containersModel,
                              ClientManagementController* clientManagementController,
                              QAppSettingsRepository* appSettingsRepository,
                              const std::shared_ptr<Settings> &settings,
                              QObject *parent = nullptr);

    Q_PROPERTY(QList<QString> qrCodes READ getQrCodes NOTIFY exportConfigChanged)
    Q_PROPERTY(int qrCodesCount READ getQrCodesCount NOTIFY exportConfigChanged)
    Q_PROPERTY(QString config READ getConfig NOTIFY exportConfigChanged)
    Q_PROPERTY(QString nativeConfigString READ getNativeConfigString NOTIFY exportConfigChanged)

public slots:
    void generateFullAccessConfig();
    void generateConnectionConfig(const QString &clientName);
    void generateOpenVpnConfig(const QString &clientName);
    void generateWireGuardConfig(const QString &clientName);
    void generateAwgConfig(const QString &clientName);
    void generateShadowSocksConfig();
    void generateCloakConfig();
    void generateXrayConfig(const QString &clientName);

    QString getConfig();
    QString getNativeConfigString();
    QList<QString> getQrCodes();

    void exportConfig(const QString &fileName);

    void updateClientManagementModel(const DockerContainer container, ServerCredentials credentials);
    void revokeConfig(const int row, const DockerContainer container, ServerCredentials credentials);
    void renameClient(const int row, const QString &clientName, const DockerContainer container, ServerCredentials credentials);

signals:
    void generateConfig(int type);
    void revokeConfigCompleted();
    void exportErrorOccurred(const QString &errorMessage);
    void exportErrorOccurred(ErrorCode errorCode);

    void exportConfigChanged();

    void saveFile(const QString &fileName, const QString &data);

private:
    int getQrCodesCount();

    void clearPreviousConfig();

    ErrorCode generateNativeConfig(const DockerContainer container, const QString &clientName, const Proto &protocol,
                                   QJsonObject &jsonNativeConfig);

    ServersController* m_serversController;
    ServersModel* m_serversModel;
    ContainersModel* m_containersModel;
    ClientManagementController* m_clientManagementController;
    QAppSettingsRepository* m_appSettingsRepository;
    std::shared_ptr<Settings> m_settings;

    QString m_config;
    QString m_nativeConfigString;
    QList<QString> m_qrCodes;
};

#endif // EXPORTCONTROLLER_H
