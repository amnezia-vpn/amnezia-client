#ifndef APICONFIGSCONTROLLER_H
#define APICONFIGSCONTROLLER_H

#include <QList>
#include <QObject>

#include "configurators/openvpn_configurator.h"
#include "ui/models/api/apiBenefitsModel.h"
#include "ui/models/api/apiServicesModel.h"
#include "ui/models/api/apiSubscriptionPlansModel.h"
#include "ui/models/servers_model.h"

class ApiConfigsController : public QObject
{
    Q_OBJECT
public:
    ApiConfigsController(const QSharedPointer<ServersModel> &serversModel, const QSharedPointer<ApiServicesModel> &apiServicesModel,
                         const QSharedPointer<ApiSubscriptionPlansModel> &subscriptionPlansModel,
                         const QSharedPointer<ApiBenefitsModel> &benefitsModel, const std::shared_ptr<Settings> &settings,
                         QObject *parent = nullptr);

    Q_INVOKABLE bool isCaptchaAwaitingUser() const;

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
    bool importService();
    bool importPremiumFromAppStore(const QString &storeProductId);
    bool restoreServiceFromAppStore();
    bool importFreeFromGateway();
    bool importTrialFromGateway(const QString &email);
    bool updateServiceFromGateway(const int serverIndex, const QString &newCountryCode, const QString &newCountryName,
                                  bool reloadServiceConfig = false);
    bool updateServiceFromTelegram(const int serverIndex);
    bool deactivateDevice(const bool isRemoveEvent);
    bool deactivateExternalDevice(const QString &uuid, const QString &serverCountryCode);

    bool isConfigValid();

    void setCurrentProtocol(const QString &protocolName);
    bool isVlessProtocol();

    void onCaptchaSolved(const QString &captchaId, const QString &solution);

signals:
    void errorOccurred(ErrorCode errorCode);
    void trialEmailError(const QString &message);
    void subscriptionExpiredOnServer();
    void subscriptionRefreshNeeded();

    void installServerFromApiFinished(const QString &message, int preferredDefaultServerIndex = -1);
    void changeApiCountryFinished(const QString &message);
    void reloadServerFromApiFinished(const QString &message);
    void updateServerFromApiFinished();

    void vpnKeyExportReady();
    void captchaRequired(const QString &captchaId, const QString &captchaImageBase64, const QString &hint);

private:
    QList<QString> getQrCodes();
    int getQrCodesCount();
    QString getVpnKey();

    ErrorCode executeRequest(const QString &endpoint, const QJsonObject &apiPayload, QByteArray &responseBody, bool isTestPurchase = false);
    ErrorCode importServiceFromBilling(const QByteArray &responseBody, const bool isTestPurchase, int &duplicateServerIndex);

    QList<QString> m_qrCodes;
    QString m_vpnKey;

    QSharedPointer<ServersModel> m_serversModel;
    QSharedPointer<ApiServicesModel> m_apiServicesModel;
    std::shared_ptr<Settings> m_settings;

    QSharedPointer<ApiSubscriptionPlansModel> m_subscriptionPlansModel;
    QSharedPointer<ApiBenefitsModel> m_benefitsModel;

    struct CaptchaState {
        QJsonObject apiPayload;
        QString endpoint;
        QString serviceProtocol;
        QString openvpnPrivKey;
        QString wireguardClientPrivKey;
        QString wireguardClientPubKey;
        QString xrayUuid;
        bool isPending = false;
    } m_captchaState;
};

#endif
