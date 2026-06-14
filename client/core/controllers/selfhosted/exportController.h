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
#include "core/repositories/secureServersRepository.h"
#include "core/repositories/secureAppSettingsRepository.h"

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

    explicit ExportController(SecureServersRepository* serversRepository,
                              SecureAppSettingsRepository* appSettingsRepository,
                              QObject *parent = nullptr);

    ExportResult generateFullAccessConfig(const QString &serverId);
    ExportResult generateConnectionConfig(const QString &serverId, int containerIndex, const QString &clientName);
    ExportResult generateOpenVpnConfig(const QString &serverId, const QString &clientName);
    ExportResult generateWireGuardConfig(const QString &serverId, const QString &clientName);
    ExportResult generateAwgConfig(const QString &serverId, int containerIndex, const QString &clientName);
    ExportResult generateXrayConfig(const QString &serverId, const QString &clientName);

signals:
    void appendClientRequested(const QString &serverId, const QString &clientId, const QString &clientName, DockerContainer container);
    void updateClientsRequested(const QString &serverId, DockerContainer container);
    void revokeClientRequested(const QString &serverId, int row, DockerContainer container);
    void renameClientRequested(const QString &serverId, int row, const QString &clientName, DockerContainer container);

public slots:
    void updateClientManagementModel(const QString &serverId, int containerIndex);
    void revokeConfig(int row, const QString &serverId, int containerIndex);
    void renameClient(int row, const QString &clientName, const QString &serverId, int containerIndex);

private:
    struct NativeConfigResult
    {
        ErrorCode errorCode = ErrorCode::NoError;
        QJsonObject jsonNativeConfig;
    };

    NativeConfigResult generateNativeConfig(const QString &serverId, DockerContainer container,
                                            const ContainerConfig &containerConfig,
                                            const QString &clientName);

    QString generateVpnUrl(const QByteArray &compressedConfig);
    QList<QString> generateQrCodesFromConfig(const QByteArray &data);
    QString generateSingleQrCode(const QByteArray &data);

    SecureServersRepository* m_serversRepository;
    SecureAppSettingsRepository* m_appSettingsRepository;
};

#endif // EXPORTCONTROLLER_H
