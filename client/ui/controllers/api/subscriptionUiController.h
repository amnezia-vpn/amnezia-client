#ifndef SUBSCRIPTIONUICONTROLLER_H
#define SUBSCRIPTIONUICONTROLLER_H

#include <QObject>

#include "core/controllers/serversController.h"
#include "core/controllers/settingsController.h"
#include "core/controllers/api/servicesCatalogController.h"
#include "core/controllers/api/subscriptionController.h"
#include "ui/models/api/apiSubscriptionPlansModel.h"
#include "ui/models/api/apiBenefitsModel.h"
#include "ui/models/api/apiServicesModel.h"
#include "ui/models/api/apiAccountInfoModel.h"
#include "ui/models/api/apiCountryModel.h"
#include "ui/models/api/apiDevicesModel.h"

class SubscriptionUiController : public QObject
{
    Q_OBJECT
public:
    SubscriptionUiController(ServersController* serversController,
                         ApiServicesModel* apiServicesModel,
                         ServicesCatalogController* servicesCatalogController,
                         SubscriptionController* subscriptionController,
                         ApiSubscriptionPlansModel* apiSubscriptionPlansModel,
                         ApiBenefitsModel* apiBenefitsModel,
                         ApiAccountInfoModel* apiAccountInfoModel,
                         ApiCountryModel* apiCountryModel,
                         ApiDevicesModel* apiDevicesModel,
                         SettingsController* settingsController,
                         QObject *parent = nullptr);

    Q_PROPERTY(QList<QString> qrCodes READ getQrCodes NOTIFY vpnKeyExportReady)
    Q_PROPERTY(int qrCodesCount READ getQrCodesCount NOTIFY vpnKeyExportReady)
    Q_PROPERTY(QString vpnKey READ getVpnKey NOTIFY vpnKeyExportReady)

public slots:
    bool exportNativeConfig(const QString &serverId, const QString &serverCountryCode, const QString &fileName);
    bool revokeNativeConfig(const QString &serverId, const QString &serverCountryCode);
    bool exportVpnKey(const QString &serverId, const QString &fileName);
    void prepareVpnKeyExport(const QString &serverId);
    void copyVpnKeyToClipboard();

    bool fillAvailableServices();
    bool importPremiumFromAppStore(const QString &storeProductId);
    bool importFreeFromGateway();
    bool restoreServiceFromAppStore();
    bool importTrialFromGateway(const QString &email);
    bool updateServiceFromGateway(const QString &serverId, const QString &newCountryCode, const QString &newCountryName,
                                  bool reloadServiceConfig = false);
    bool deactivateDevice(const QString &serverId);
    bool deactivateExternalDevice(const QString &serverId, const QString &uuid, const QString &serverCountryCode);

    void validateConfig();

    void setCurrentProtocol(const QString &serverId, const QString &protocolName);
    bool isVlessProtocol(const QString &serverId);

    void removeApiConfig(const QString &serverId);

    void removeServer(const QString &serverId);

    bool getAccountInfo(const QString &serverId, bool reload);
    void getRenewalLink(const QString &serverId);

    void updateApiCountryModel();
    void updateApiDevicesModel();

signals:
    void configValidated(bool isValid);
    void errorOccurred(ErrorCode errorCode);
    void trialEmailError(const QString &message);
    void subscriptionExpiredOnServer();
    void renewalLinkReceived(const QString &url);

    void installServerFromApiFinished(const QString &message, int preferredDefaultServerIndex = -1);
    void changeApiCountryFinished(const QString &message);
    void reloadServerFromApiFinished(const QString &message);
    void updateServerFromApiFinished();
    void subscriptionRefreshNeeded();

    void apiConfigRemoved(const QString &message);
    void apiServerRemoved(const QString &message);

    void vpnKeyExportReady();

    void unsupportedConnectDrawerRequested();

private:
    QList<QString> getQrCodes();
    int getQrCodesCount();
    QString getVpnKey();

    QList<QString> m_qrCodes;
    QString m_vpnKey;

    ServersController* m_serversController;
    ApiServicesModel* m_apiServicesModel;
    ServicesCatalogController* m_servicesCatalogController;
    SubscriptionController* m_subscriptionController;
    ApiSubscriptionPlansModel* m_apiSubscriptionPlansModel;
    ApiBenefitsModel* m_apiBenefitsModel;
    ApiAccountInfoModel* m_apiAccountInfoModel;
    ApiCountryModel* m_apiCountryModel;
    ApiDevicesModel* m_apiDevicesModel;
    SettingsController* m_settingsController;
};

#endif // SUBSCRIPTIONUICONTROLLER_H
