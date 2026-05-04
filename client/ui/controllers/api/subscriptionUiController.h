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
    bool exportNativeConfig(int serverIndex, const QString &serverCountryCode, const QString &fileName);
    bool revokeNativeConfig(int serverIndex, const QString &serverCountryCode);
    bool exportVpnKey(int serverIndex, const QString &fileName);
    void prepareVpnKeyExport(int serverIndex);
    void copyVpnKeyToClipboard();

    bool fillAvailableServices();
    bool importPremiumFromAppStore(const QString &storeProductId);
    bool importFreeFromGateway();
    bool restoreServiceFromAppStore();
    bool importTrialFromGateway(const QString &email);
    bool updateServiceFromGateway(const int serverIndex, const QString &newCountryCode, const QString &newCountryName,
                                  bool reloadServiceConfig = false);
    bool updateServiceFromTelegram(const int serverIndex);
    bool deactivateDevice(int serverIndex);
    bool deactivateExternalDevice(int serverIndex, const QString &uuid, const QString &serverCountryCode);

    void validateConfig();

    void setCurrentProtocol(int serverIndex, const QString &protocolName);
    bool isVlessProtocol(int serverIndex);

    void removeApiConfig(int serverIndex);

    bool getAccountInfo(int serverIndex, bool reload);
    void getRenewalLink(int serverIndex);
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

    void vpnKeyExportReady();

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
