#ifndef EXPORTCONTROLLER_H
#define EXPORTCONTROLLER_H

#include <QObject>
#include <QJsonObject>
#include <QList>
#include <QString>

#include "containers/containers_defs.h"
#include "core/utils/defs.h"
#include "core/repositories/serversRepository.h"
#include "core/repositories/appSettingsRepository.h"
#include "settings.h"

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

    explicit ExportController(ServersRepository* serversRepository,
                              AppSettingsRepository* appSettingsRepository,
                              const std::shared_ptr<Settings> &settings,
                              QObject *parent = nullptr);

    ExportResult generateFullAccessConfig(int serverIndex);
    ExportResult generateConnectionConfig(int serverIndex, int containerIndex, const QString &clientName);
    ExportResult generateOpenVpnConfig(int serverIndex, int containerIndex, const QString &clientName, bool isApiConfig);
    ExportResult generateWireGuardConfig(int serverIndex, const QString &clientName, bool isApiConfig);
    ExportResult generateAwgConfig(int serverIndex, int containerIndex, const QString &clientName, bool isApiConfig);
    ExportResult generateXrayConfig(int serverIndex, const QString &clientName, bool isApiConfig);

signals:
    void appendClientRequested(DockerContainer container, const ServerCredentials &credentials,
                              const QJsonObject &containerConfig, const QString &clientName);
    void appendClientByConfigRequested(QJsonObject protocolConfig, const QString &clientName,
                                       DockerContainer container, const ServerCredentials &credentials);
    void updateClientsRequested(DockerContainer container, const ServerCredentials &credentials);
    void revokeClientRequested(int row, DockerContainer container, const ServerCredentials &credentials,
                              int serverIndex);
    void renameClientRequested(int row, const QString &clientName, DockerContainer container,
                              const ServerCredentials &credentials);

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
                                            const QJsonObject &containerConfig,
                                            const QString &clientName, Proto protocol,
                                            bool isApiConfig);

    QString generateVpnUrl(const QByteArray &compressedConfig);
    QList<QString> generateQrCodesFromConfig(const QByteArray &data);
    QString generateSingleQrCode(const QByteArray &data);
    QPair<QString, QString> getDnsPair(int serverIndex, bool isAmneziaDnsEnabled) const;

    ServersRepository* m_serversRepository;
    AppSettingsRepository* m_appSettingsRepository;
    std::shared_ptr<Settings> m_settings;
};

#endif // EXPORTCONTROLLER_H
