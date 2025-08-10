#ifndef APICONFIGSCONTROLLER_H
#define APICONFIGSCONTROLLER_H

#include <QObject>

#include "configurators/openvpn_configurator.h"
#include "ui/models/api/apiServicesModel.h"
#include "ui/models/servers_model.h"

class ApiConfigsController : public QObject
{
    Q_OBJECT
public:
    ApiConfigsController(const QSharedPointer<ServersModel> &serversModel, const QSharedPointer<ApiServicesModel> &apiServicesModel,
                         const std::shared_ptr<Settings> &settings, QObject *parent = nullptr);

    QList<QString> getQrCodes();
    int getQrCodesCount();
    QString getVpnKey();

public slots:
    ErrorCode exportNativeConfig(const QString &serverCountryCode, const QString &fileName);
    ErrorCode revokeNativeConfig(const QString &serverCountryCode);
    void prepareVpnKeyExport();
    void copyVpnKeyToClipboard();

    ErrorCode fillAvailableServices();
    ErrorCode importServiceFromGateway();
    ErrorCode updateServiceFromGateway(const int serverIndex, const QString &newCountryCode, const QString &newCountryName,
                                       bool reloadServiceConfig = false);
    ErrorCode updateServiceFromTelegram(const int serverIndex);
    ErrorCode deactivateDevice();
    ErrorCode deactivateExternalDevice(const QString &uuid, const QString &serverCountryCode);

    bool isConfigValid();

    void setCurrentProtocol(const QString &protocolName);
    bool isVlessProtocol();

private:
    ErrorCode executeRequest(const QString &endpoint, const QJsonObject &apiPayload, QByteArray &responseBody);

      bool isServerFromApiAlreadyExists(const QString &userCountryCode,
                                        const QString &serviceType,
                                        const QString &serviceProtocol) const;
      bool isApiKeyExpired(int serverIndex) const;
      void removeApiConfig(int serverIndex);

    QList<QString> m_qrCodes;
    QString m_vpnKey;

    QSharedPointer<ServersModel> m_serversModel;
    QSharedPointer<ApiServicesModel> m_apiServicesModel;
    std::shared_ptr<Settings> m_settings;
};

#endif // APICONFIGSCONTROLLER_H
