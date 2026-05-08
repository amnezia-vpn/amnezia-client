#ifndef PAIRINGUICONTROLLER_H
#define PAIRINGUICONTROLLER_H

#include <QFutureWatcher>
#include <QNetworkReply>
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
    /** Long-poll window for generate_qr (seconds), for receive UI countdown. */
    Q_PROPERTY(int tvPairingWaitWindowSeconds READ tvPairingWaitWindowSeconds NOTIFY tvQrCodesChanged)

    Q_PROPERTY(bool phonePairingBusy READ phonePairingBusy NOTIFY phonePairingBusyChanged)
    Q_PROPERTY(QString phoneStatusMessage READ phoneStatusMessage NOTIFY phoneStatusMessageChanged)
    Q_PROPERTY(QString pendingPhonePairingUuid READ pendingPhonePairingUuid WRITE setPendingPhonePairingUuid NOTIFY
                       pendingPhonePairingUuidChanged)
    Q_PROPERTY(QString lastSuccessfulPhonePairingDisplayName READ lastSuccessfulPhonePairingDisplayName NOTIFY
                       lastSuccessfulPhonePairingDisplayNameChanged)
    /** TV flow for QA: 0=idle, 1=waitingForPeer, 2=error, 3=sessionExpired */
    Q_PROPERTY(int tvPairingUiPhase READ tvPairingUiPhase NOTIFY tvPairingUiPhaseChanged)

public:
    PairingUiController(PairingController *pairingController, ServersController *serversController,
                        SubscriptionController *subscriptionController, SecureAppSettingsRepository *appSettingsRepository,
                        QObject *parent = nullptr);
    ~PairingUiController() override;

    QVariantList tvQrCodes() const;
    int tvQrCodesCount() const;
    QString tvSessionUuid() const;
    bool tvPairingBusy() const;
    QString tvStatusMessage() const;
    int tvPairingWaitWindowSeconds() const;

    bool phonePairingBusy() const;
    QString phoneStatusMessage() const;
    QString pendingPhonePairingUuid() const { return m_pendingPhonePairingUuid; }
    void setPendingPhonePairingUuid(const QString &uuid);
    QString lastSuccessfulPhonePairingDisplayName() const { return m_lastSuccessfulPhonePairingDisplayName; }
    int tvPairingUiPhase() const { return m_tvPairingUiPhase; }

#if defined(Q_OS_ANDROID)
    static bool tryConsumeAndroidQrScan(const QString &code);
#endif

public slots:
    void startTvQrSession();
    void cancelTvQrSession();
    /** TV receive + phone send: call when leaving QR pairing (back / pop) so long-poll state does not stick. */
    void cancelAllPairingActivity();

    /** Sends the current premium/free API config from \a serverIndex to the gateway for the given \a qrUuid. */
    void submitPhonePairing(const QString &qrUuid, int serverIndex);

    /** Android: system camera activity. iOS: toggle camera from QML. */
    void openPairingQrScanner();

    /** If \a raw contains a session UUID (not vpn://), emits pairingUuidFromScan and returns true. */
    bool applyScannedTextAsPairingUuid(const QString &raw);

    Q_INVOKABLE void clearPendingPhonePairingUuid();

signals:
    void errorOccurred(amnezia::ErrorCode errorCode);
    void tvQrCodesChanged();
    void tvSessionUuidChanged();
    void tvPairingBusyChanged();
    void tvStatusMessageChanged();
    void phonePairingBusyChanged();
    void phoneStatusMessageChanged();
    void pendingPhonePairingUuidChanged();
    void lastSuccessfulPhonePairingDisplayNameChanged();

    void tvPairingConfigReceived();
    void phonePairingSucceeded();
    /** scan_qr rejected: subscription device quota full (no generic error dialog). */
    void phonePairingRejectedDeviceLimit();

    void pairingUuidFromScan(const QString &uuid);
    void tvPairingUiPhaseChanged();

private:
    void setTvBusy(bool busy);
    void setPhoneBusy(bool busy);
    void resetTvQrDisplay();
    QString tvFailureMessage(amnezia::ErrorCode code) const;
    void dispatchTvGenerateQrAttempt(quint64 generation, int retryAttempt);
    void dispatchPhoneScanQrAttempt(const QString &qrUuid, bool isTestPurchase, const QString &vpnKey, const QJsonObject &serviceInfo,
                                    const QJsonArray &supportedProtocols, const QString &apiKey, quint64 generation, int retryAttempt);
    void setTvPairingUiPhase(int phase);

    PairingController *m_pairingController {};
    ServersController *m_serversController {};
    SubscriptionController *m_subscriptionController {};
    SecureAppSettingsRepository *m_appSettingsRepository {};

    QList<QString> m_tvQrCodes;
    QString m_tvSessionUuid;
    bool m_tvPairingBusy = false;
    QString m_tvStatusMessage;
    QPointer<QFutureWatcher<QPair<amnezia::ErrorCode, QByteArray>>> m_tvWatcher;
    QPointer<QNetworkReply> m_tvNetworkReply;
    quint64 m_tvSessionGeneration { 0 };
    int m_tvPairingUiPhase { 0 };

    bool m_phonePairingBusy = false;
    QString m_phoneStatusMessage;
    QString m_pendingPhonePairingUuid;
    QString m_lastSuccessfulPhonePairingDisplayName;
    QPointer<QFutureWatcher<QPair<amnezia::ErrorCode, QByteArray>>> m_phoneWatcher;
    QPointer<QNetworkReply> m_phoneNetworkReply;
    quint64 m_phoneSessionGeneration { 0 };
};

#endif // PAIRINGUICONTROLLER_H
