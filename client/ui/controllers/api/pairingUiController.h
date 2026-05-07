#ifndef PAIRINGUICONTROLLER_H
#define PAIRINGUICONTROLLER_H

#include <QFutureWatcher>
#include <QObject>
#include <QVariantList>
#include <QPointer>
#include <QStringList>

#include "core/controllers/api/pairingController.h"
#include "core/controllers/api/subscriptionController.h"
#include "core/controllers/serversController.h"
#include "core/repositories/secureAppSettingsRepository.h"

#include "core/utils/errorCodes.h"

class PairingUiController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QVariantList tvQrCodes READ tvQrCodes NOTIFY tvQrCodesChanged)
    Q_PROPERTY(int tvQrCodesCount READ tvQrCodesCount NOTIFY tvQrCodesChanged)
    Q_PROPERTY(QString tvSessionUuid READ tvSessionUuid NOTIFY tvSessionUuidChanged)
    Q_PROPERTY(bool tvPairingBusy READ tvPairingBusy NOTIFY tvPairingBusyChanged)
    Q_PROPERTY(QString tvStatusMessage READ tvStatusMessage NOTIFY tvStatusMessageChanged)

    Q_PROPERTY(bool phonePairingBusy READ phonePairingBusy NOTIFY phonePairingBusyChanged)
    Q_PROPERTY(QString phoneStatusMessage READ phoneStatusMessage NOTIFY phoneStatusMessageChanged)

public:
    PairingUiController(PairingController *pairingController, ServersController *serversController,
                        SubscriptionController *subscriptionController, SecureAppSettingsRepository *appSettingsRepository,
                        QObject *parent = nullptr);

    QVariantList tvQrCodes() const;
    int tvQrCodesCount() const;
    QString tvSessionUuid() const;
    bool tvPairingBusy() const;
    QString tvStatusMessage() const;

    bool phonePairingBusy() const;
    QString phoneStatusMessage() const;

public slots:
    void startTvQrSession();
    void cancelTvQrSession();

    /** Sends the current premium/free API config from \a serverIndex to the gateway for the given \a qrUuid. */
    void submitPhonePairing(const QString &qrUuid, int serverIndex);

signals:
    void errorOccurred(amnezia::ErrorCode errorCode);
    void tvQrCodesChanged();
    void tvSessionUuidChanged();
    void tvPairingBusyChanged();
    void tvStatusMessageChanged();
    void phonePairingBusyChanged();
    void phoneStatusMessageChanged();

    void tvPairingConfigReceived();
    void phonePairingSucceeded();

private:
    void setTvBusy(bool busy);
    void setPhoneBusy(bool busy);
    void resetTvQrDisplay();
    void runPhonePairingRequest(const QString &qrUuid, bool isTestPurchase, const QString &vpnKey, const QJsonObject &serviceInfo,
                                const QJsonArray &supportedProtocols, const QString &apiKey);

    PairingController *m_pairingController {};
    ServersController *m_serversController {};
    SubscriptionController *m_subscriptionController {};
    SecureAppSettingsRepository *m_appSettingsRepository {};

    QList<QString> m_tvQrCodes;
    QString m_tvSessionUuid;
    bool m_tvPairingBusy = false;
    QString m_tvStatusMessage;
    QPointer<QFutureWatcher<QPair<amnezia::ErrorCode, QByteArray>>> m_tvWatcher;

    bool m_phonePairingBusy = false;
    QString m_phoneStatusMessage;
    QPointer<QFutureWatcher<QPair<amnezia::ErrorCode, QByteArray>>> m_phoneWatcher;
};

#endif // PAIRINGUICONTROLLER_H
