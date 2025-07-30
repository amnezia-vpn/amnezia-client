#ifndef EXPORTUICONTROLLER_H
#define EXPORTUICONTROLLER_H

#include <QObject>

#include "ui/models/selfhosted/clientManagementModel.h"
#include "ui/models/containers_model.h"
#include "ui/models/servers_model.h"
#include "core/controllers/selfhosted/exportController.h"
#include "core/controllers/selfhosted/clientManagementController.h"

class ExportUIController : public QObject
{
    Q_OBJECT
public:
    explicit ExportUIController(const QSharedPointer<ServersModel> &serversModel, const QSharedPointer<ContainersModel> &containersModel,
                                const QSharedPointer<ClientManagementModel> &clientManagementModel, 
                                QSharedPointer<ExportController> coreExportController,
                                QSharedPointer<ClientManagementController> clientManagementController,
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
    void exportErrorOccurred(const QString &errorMessage);
    void exportErrorOccurred(ErrorCode errorCode);

    void exportConfigChanged();

    void saveFile(const QString &fileName, const QString &data);

private:
    int getQrCodesCount();

    void clearPreviousConfig();

    QSharedPointer<ServersModel> m_serversModel;
    QSharedPointer<ContainersModel> m_containersModel;
    QSharedPointer<ClientManagementModel> m_clientManagementModel;
    QSharedPointer<ExportController> m_coreExportController;
    QSharedPointer<ClientManagementController> m_clientManagementController;

    QString m_config;
    QString m_nativeConfigString;
    QList<QString> m_qrCodes;
};

#endif // EXPORTUICONTROLLER_H
