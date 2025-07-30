#ifndef EXPORT_CORE_CONTROLLER_H
#define EXPORT_CORE_CONTROLLER_H

#include <QObject>
#include <QSharedPointer>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QString>

#include "core/defs.h"
#include "core/models/containers/containers_defs.h"

class ServerController;
class Settings;

using namespace amnezia;

struct ExportConfigResult
{
    QString config;
    QString nativeConfigString;
    QList<QString> qrCodes;
    ErrorCode errorCode;
};

class ExportController : public QObject
{
    Q_OBJECT

public:
    explicit ExportController(std::shared_ptr<Settings> settings, 
                              QObject *parent = nullptr);

signals:
    void clientAppendRequested(const DockerContainer container, const ServerCredentials &credentials, 
                              const QJsonObject &containerConfig, const QString &clientName, 
                              const QSharedPointer<ServerController> &serverController);
    
    void nativeConfigClientAppendRequested(const QJsonObject &jsonNativeConfig, const QString &clientName, 
                                          const DockerContainer container, const ServerCredentials &credentials, 
                                          const QSharedPointer<ServerController> &serverController);

    void configGenerated(const ExportConfigResult &result);
    void clientManagementError(ErrorCode errorCode);

public slots:
    void onClientAppendCompleted(ErrorCode errorCode);
    void onNativeConfigClientAppendCompleted(ErrorCode errorCode);

    ExportConfigResult generateFullAccessConfig(const QJsonObject &serverConfig);
    
    ExportConfigResult generateConnectionConfig(const QString &clientName, 
                                               const ServerCredentials &credentials,
                                               const DockerContainer container,
                                               const QJsonObject &containerConfig,
                                               const QJsonObject &serverConfig,
                                               const QPair<QString, QString> &dnsSettings);
    
    ExportConfigResult generateOpenVpnConfig(const QString &clientName,
                                            const ServerCredentials &credentials,
                                            const DockerContainer container,
                                            const QJsonObject &containerConfig,
                                            const QPair<QString, QString> &dnsSettings,
                                            bool isApiConfig = false);
    
    ExportConfigResult generateWireGuardConfig(const QString &clientName,
                                              const ServerCredentials &credentials,
                                              const QJsonObject &containerConfig,
                                              const QPair<QString, QString> &dnsSettings,
                                              bool isApiConfig = false);
    
    ExportConfigResult generateAwgConfig(const QString &clientName,
                                        const ServerCredentials &credentials,
                                        const QJsonObject &containerConfig,
                                        const QPair<QString, QString> &dnsSettings,
                                        bool isApiConfig = false);
    
    ExportConfigResult generateShadowSocksConfig(const ServerCredentials &credentials,
                                                const DockerContainer container,
                                                const QJsonObject &containerConfig,
                                                const QPair<QString, QString> &dnsSettings,
                                                bool isApiConfig = false);
    
    ExportConfigResult generateCloakConfig(const ServerCredentials &credentials,
                                          const QJsonObject &containerConfig,
                                          const QPair<QString, QString> &dnsSettings,
                                          bool isApiConfig = false);
    
    ExportConfigResult generateXrayConfig(const QString &clientName,
                                         const ServerCredentials &credentials,
                                         const QJsonObject &containerConfig,
                                         const QPair<QString, QString> &dnsSettings,
                                         bool isApiConfig = false);

private:
    ErrorCode generateNativeConfig(const DockerContainer container, const QString &clientName, 
                                  const Proto &protocol, const ServerCredentials &credentials,
                                  const QJsonObject &containerConfig, const QPair<QString, QString> &dnsSettings,
                                  bool isApiConfig, QJsonObject &jsonNativeConfig,
                                  const QSharedPointer<ServerController> &serverController);
    
    QList<QString> generateQrCodeSeries(const QByteArray &data);
    QString generateQrCode(const QByteArray &data);
    
    std::shared_ptr<Settings> m_settings;
    
    ErrorCode m_lastClientAppendResult;
    ErrorCode m_lastNativeConfigAppendResult;
    bool m_waitingForClientAppend;
    bool m_waitingForNativeConfigAppend;
};

#endif // EXPORT_CORE_CONTROLLER_H 
