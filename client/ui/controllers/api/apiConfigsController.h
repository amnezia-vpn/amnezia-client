#ifndef APICONFIGSCONTROLLER_H
#define APICONFIGSCONTROLLER_H

#include <QObject>

#include "configurators/openvpn_configurator.h"
#include "core/controllers/serversController.h"
#include "core/controllers/api/servicesCatalogController.h"
#include "core/controllers/api/subscriptionController.h"
#include "ui/models/api/apiServicesModel.h"
#include "ui/models/servers_model.h"

class ApiConfigsController : public QObject
{
    Q_OBJECT
public:
    ApiConfigsController(ServersController* serversController,
                         ServersModel* serversModel, ApiServicesModel* apiServicesModel,
                         ServicesCatalogController* servicesCatalogController,
                         SubscriptionController* subscriptionController,
                         QObject *parent = nullptr);

    Q_PROPERTY(QList<QString> qrCodes READ getQrCodes NOTIFY vpnKeyExportReady)
    Q_PROPERTY(int qrCodesCount READ getQrCodesCount NOTIFY vpnKeyExportReady)
    Q_PROPERTY(QString vpnKey READ getVpnKey NOTIFY vpnKeyExportReady)

public slots:
    bool exportNativeConfig(const QString &serverCountryCode, const QString &fileName);
    bool revokeNativeConfig(const QString &serverCountryCode);
    bool exportVpnKey(const QString &fileName);
    void prepareVpnKeyExport();
    void copyVpnKeyToClipboard();

    bool fillAvailableServices();
    bool importServiceFromGateway();
    bool updateServiceFromGateway(const int serverIndex, const QString &newCountryCode, const QString &newCountryName,
                                  bool reloadServiceConfig = false);
    bool updateServiceFromTelegram(const int serverIndex);
    bool deactivateDevice(const bool isRemoveEvent);
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

    QList<QString> m_qrCodes;
    QString m_vpnKey;

    ServersController* m_serversController;
    ServersModel* m_serversModel;
    ApiServicesModel* m_apiServicesModel;
    ServicesCatalogController* m_servicesCatalogController;
    SubscriptionController* m_subscriptionController;
};

#endif // APICONFIGSCONTROLLER_H
