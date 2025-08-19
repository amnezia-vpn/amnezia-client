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

    Q_PROPERTY(QList<QString> qrCodes READ getQrCodes NOTIFY vpnKeyExportReady)
    Q_PROPERTY(int qrCodesCount READ getQrCodesCount NOTIFY vpnKeyExportReady)
    Q_PROPERTY(QString vpnKey READ getVpnKey NOTIFY vpnKeyExportReady)

public slots:
    bool exportNativeConfig(const QString &serverCountryCode, const QString &fileName);
    bool revokeNativeConfig(const QString &serverCountryCode);
    // bool exportVpnKey(const QString &fileName);
    void prepareVpnKeyExport();
    void copyVpnKeyToClipboard();

    bool fillAvailableServices();
    bool importServiceFromGateway();
    bool updateServiceFromGateway(const int serverIndex, const QString &newCountryCode, const QString &newCountryName,
                                  bool reloadServiceConfig = false);
    bool updateServiceFromTelegram(const int serverIndex);
    bool deactivateDevice();
    bool deactivateExternalDevice(const QString &uuid, const QString &serverCountryCode);

    bool isConfigValid();

    void setCurrentProtocol(const QString &protocolName);
    bool isVlessProtocol();

signals:
    void errorOccurred(ErrorCode errorCode);

    void installServerFromApiFinished(const QString &message);
    void changeApiCountryFinished(const QString &message);
    void reloadServerFromApiFinished(const QString &message);
    void updateServerFromApiFinished();

    void vpnKeyExportReady();

private:
    QList<QString> getQrCodes();
    int getQrCodesCount();
    QString getVpnKey();

    ErrorCode executeRequest(const QString &endpoint, const QJsonObject &apiPayload, QByteArray &responseBody);

    QList<QString> m_qrCodes;
    QString m_vpnKey;

    QSharedPointer<ServersModel> m_serversModel;
    QSharedPointer<ApiServicesModel> m_apiServicesModel;
    std::shared_ptr<Settings> m_settings;
};

#endif // APICONFIGSCONTROLLER_H
