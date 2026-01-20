#ifndef EXPORTCONTROLLER_H
#define EXPORTCONTROLLER_H

#include <QObject>
#include <QJsonObject>
#include <QList>
#include <QString>

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"
#include "core/repositories/qServersRepository.h"
#include "core/repositories/qAppSettingsRepository.h"

class SshSession;
class VpnConfigurationsController;

using namespace amnezia;

class ExportController : public QObject
{
    Q_OBJECT

public:
    struct ExportResult
    {
        ErrorCode errorCode = ErrorCode::NoError;
        QString config;
        QString nativeConfigString;
        QList<QString> qrCodes;
    };

    explicit ExportController(QServersRepository* serversRepository,
                              QAppSettingsRepository* appSettingsRepository,
                              QObject *parent = nullptr);

    ExportResult generateFullAccessConfig(int serverIndex);
    ExportResult generateConnectionConfig(int serverIndex, int containerIndex, const QString &clientName);
    ExportResult generateOpenVpnConfig(int serverIndex, int containerIndex, const QString &clientName, bool isApiConfig);
    ExportResult generateWireGuardConfig(int serverIndex, const QString &clientName, bool isApiConfig);
    ExportResult generateAwgConfig(int serverIndex, int containerIndex, const QString &clientName, bool isApiConfig);
    ExportResult generateXrayConfig(int serverIndex, const QString &clientName, bool isApiConfig);

signals:
    void appendClientRequested(int serverIndex, const QString &clientId, const QString &clientName, DockerContainer container);
    void updateClientsRequested(int serverIndex, DockerContainer container);
    void revokeClientRequested(int serverIndex, int row, DockerContainer container);
    void renameClientRequested(int serverIndex, int row, const QString &clientName, DockerContainer container);

public slots:
    void updateClientManagementModel(int serverIndex, int containerIndex);
    void revokeConfig(int row, int serverIndex, int containerIndex);
    void renameClient(int row, const QString &clientName, int serverIndex, int containerIndex);

private:
    struct NativeConfigResult
    {
        ErrorCode errorCode = ErrorCode::NoError;
        QJsonObject jsonNativeConfig;
    };

    NativeConfigResult generateNativeConfig(int serverIndex, DockerContainer container,
                                            const ContainerConfig &containerConfig,
                                            const QString &clientName, Proto protocol,
                                            bool isApiConfig);

    QString generateVpnUrl(const QByteArray &compressedConfig);
    QList<QString> generateQrCodesFromConfig(const QByteArray &data);
    QString generateSingleQrCode(const QByteArray &data);

    QServersRepository* m_serversRepository;
    QAppSettingsRepository* m_appSettingsRepository;
};

#endif // EXPORTCONTROLLER_H
