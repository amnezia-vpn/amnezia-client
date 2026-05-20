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
    Q_PROPERTY(int tvPairingWaitWindowSeconds READ tvPairingWaitWindowSeconds NOTIFY tvQrCodesChanged)

    Q_PROPERTY(bool phonePairingBusy READ phonePairingBusy NOTIFY phonePairingBusyChanged)
    Q_PROPERTY(QString pendingPhonePairingUuid READ pendingPhonePairingUuid WRITE setPendingPhonePairingUuid NOTIFY
                       pendingPhonePairingUuidChanged)
    Q_PROPERTY(QString lastSuccessfulPhonePairingDisplayName READ lastSuccessfulPhonePairingDisplayName NOTIFY
                       lastSuccessfulPhonePairingDisplayNameChanged)
    Q_PROPERTY(qint64 androidPairingReaderCooldownUntilEpochMs READ androidPairingReaderCooldownUntilEpochMs NOTIFY
                       androidPairingReaderCooldownUntilEpochMsChanged)

public:
    PairingUiController(PairingController *pairingController, ServersController *serversController,
                        SubscriptionController *subscriptionController, SecureAppSettingsRepository *appSettingsRepository,
                        QObject *parent = nullptr);
    ~PairingUiController() override;

    QVariantList tvQrCodes() const;
    int tvQrCodesCount() const;
    int tvPairingWaitWindowSeconds() const;

    bool phonePairingBusy() const;
    QString pendingPhonePairingUuid() const { return m_pendingPhonePairingUuid; }
    void setPendingPhonePairingUuid(const QString &uuid);
    QString lastSuccessfulPhonePairingDisplayName() const { return m_lastSuccessfulPhonePairingDisplayName; }

    qint64 androidPairingReaderCooldownUntilEpochMs() const { return m_androidPairingReaderCooldownUntilEpochMs; }

    Q_INVOKABLE void presentIosPairingQrNativeOverlayScanner(const QString &title = QString(),
                                                           const QString &subtitle = QString());
    Q_INVOKABLE void dismissIosPairingQrNativeOverlayScanner();
    Q_INVOKABLE void restartIosPairingQrNativeOverlayCapture();

#if defined(Q_OS_ANDROID)
    static bool tryConsumeAndroidQrScan(const QString &code);
    static void notifyAndroidPairingQrCameraClosed();
    static void notifyAndroidPairingQrCameraUserDismissed();
#endif

public slots:
    bool canOpenTvQrPairingPage();
    void startTvQrSession();
    /** New UUID + QR image; aborts in-flight generate_qr without clearing the code first. */
    void rotateTvQrSession();
    void cancelTvQrSession();
    void cancelAllPairingActivity();

    void submitPhonePairing(const QString &qrUuid, int serverIndex);

    void openPairingQrScanner();

    Q_INVOKABLE bool isPairingCameraAccessGranted() const;
    Q_INVOKABLE void requestPairingCameraAccess();
    Q_INVOKABLE void openPairingCameraAppSettings();
    Q_INVOKABLE void setPairingQrTorchEnabled(bool enabled);

    bool applyScannedTextAsPairingUuid(const QString &raw);

signals:
    void errorOccurred(amnezia::ErrorCode errorCode);
    void tvQrCodesChanged();
    void phonePairingBusyChanged();
    void pendingPhonePairingUuidChanged();
    void lastSuccessfulPhonePairingDisplayNameChanged();

    void tvPairingConfigReceived();
    void tvPairingConfigAlreadyAdded();
    void phonePairingSucceeded();

    void pairingUuidFromScan(const QString &uuid);
    void pairingCameraAccessFinished(bool granted);
    void androidPairingReaderCooldownUntilEpochMsChanged();
    void pairingSendQrScanRejectedInvalidPayload();
    void pairingIosNativeQrOverlayBackRequested();
    void pairingAndroidNativeQrScannerUserDismissed();

private:
    void setTvBusy(bool busy);
    void setPhoneBusy(bool busy);
    void resetTvQrDisplay();
    void clearPendingPhonePairingUuid();
    void suppressAndroidNativePairingReaderStarts(int ms);
    void dispatchTvGenerateQrAttempt(quint64 generation, int retryAttempt);
    void dispatchPhoneScanQrAttempt(const QString &qrUuid, bool isTestPurchase, const QString &vpnKey, const QJsonObject &serviceInfo,
                                    const QJsonArray &supportedProtocols, const QString &apiKey, const QString &serviceType,
                                    const QString &userCountryCode, quint64 generation, int retryAttempt);

    PairingController *m_pairingController {};
    ServersController *m_serversController {};
    SubscriptionController *m_subscriptionController {};
    SecureAppSettingsRepository *m_appSettingsRepository {};

    QList<QString> m_tvQrCodes;
    QString m_tvSessionUuid;
    bool m_tvPairingBusy = false;
    QPointer<QFutureWatcher<QPair<amnezia::ErrorCode, QByteArray>>> m_tvWatcher;
    QPointer<QNetworkReply> m_tvNetworkReply;
    quint64 m_tvSessionGeneration { 0 };

    bool m_phonePairingBusy = false;
    QString m_pendingPhonePairingUuid;
    QString m_lastSuccessfulPhonePairingDisplayName;
    QPointer<QFutureWatcher<QPair<amnezia::ErrorCode, QByteArray>>> m_phoneWatcher;
    QPointer<QNetworkReply> m_phoneNetworkReply;
    quint64 m_phoneSessionGeneration { 0 };

    qint64 m_androidPairingReaderCooldownUntilEpochMs = 0;
};

#endif // PAIRINGUICONTROLLER_H
