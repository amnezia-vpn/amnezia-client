#ifndef SUBSCRIPTIONUICONTROLLER_H
#define SUBSCRIPTIONUICONTROLLER_H

#include <QObject>

#include "core/controllers/serversController.h"
#include "core/controllers/settingsController.h"
#include "core/controllers/api/servicesCatalogController.h"
#include "core/controllers/api/subscriptionController.h"
#include "ui/models/api/apiServicesModel.h"
#include "ui/models/api/apiAccountInfoModel.h"
#include "ui/models/api/apiCountryModel.h"
#include "ui/models/api/apiDevicesModel.h"
#include "ui/models/serversModel.h"

class SubscriptionUiController : public QObject
{
    Q_OBJECT
public:
    SubscriptionUiController(ServersController* serversController,
                         ServersModel* serversModel, ApiServicesModel* apiServicesModel,
                         ServicesCatalogController* servicesCatalogController,
                         SubscriptionController* subscriptionController,
                         ApiAccountInfoModel* apiAccountInfoModel,
                         ApiCountryModel* apiCountryModel,
                         ApiDevicesModel* apiDevicesModel,
                         SettingsController* settingsController,
                         QObject *parent = nullptr);

    Q_PROPERTY(QList<QString> qrCodes READ getQrCodes NOTIFY vpnKeyExportReady)
    Q_PROPERTY(int qrCodesCount READ getQrCodesCount NOTIFY vpnKeyExportReady)
    Q_PROPERTY(QString vpnKey READ getVpnKey NOTIFY vpnKeyExportReady)

public slots:
    bool exportNativeConfig(int serverIndex, const QString &serverCountryCode, const QString &fileName);
    bool revokeNativeConfig(int serverIndex, const QString &serverCountryCode);
    bool exportVpnKey(int serverIndex, const QString &fileName);
    void prepareVpnKeyExport(int serverIndex);
    void copyVpnKeyToClipboard();

    bool fillAvailableServices();
    bool importService();
    bool importSerivceFromAppStore();
    bool restoreSerivceFromAppStore();
    bool importServiceFromGateway();
    bool updateServiceFromGateway(const int serverIndex, const QString &newCountryCode, const QString &newCountryName,
                                  bool reloadServiceConfig = false);
    bool updateServiceFromTelegram(const int serverIndex);
    bool deactivateDevice(int serverIndex, const bool isRemoveEvent);
    bool deactivateExternalDevice(const QString &uuid, const QString &serverCountryCode);

    bool isConfigValid();

    void setCurrentProtocol(int serverIndex, const QString &protocolName);
    bool isVlessProtocol(int serverIndex);

    void removeApiConfig(int serverIndex);

    bool getAccountInfo(int serverIndex, bool reload);
    void updateApiCountryModel();
    void updateApiDevicesModel();

signals:
    void errorOccurred(ErrorCode errorCode);

    void installServerFromApiFinished(const QString &message);
    void changeApiCountryFinished(const QString &message);
    void reloadServerFromApiFinished(const QString &message);
    void updateServerFromApiFinished();

    void apiConfigRemoved(const QString &message);

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
    ApiAccountInfoModel* m_apiAccountInfoModel;
    ApiCountryModel* m_apiCountryModel;
    ApiDevicesModel* m_apiDevicesModel;
    SettingsController* m_settingsController;
};

#endif // SUBSCRIPTIONUICONTROLLER_H
