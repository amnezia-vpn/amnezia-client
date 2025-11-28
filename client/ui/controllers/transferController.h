#ifndef TRANSFERCONTROLLER_H
#define TRANSFERCONTROLLER_H

#include <QObject>
#include <QScopedPointer>
#include <QJsonObject>
#include <QJsonDocument>
#include <QUuid>
#include <QAtomicInt>
#include <QFuture>

#include "core/qrCodeUtils.h"

class Settings;
class ServersModel;
class ExportController;
class ImportController;

class TransferController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString qrCodeUrl READ qrCodeUrl NOTIFY qrCodeUpdated)
    Q_PROPERTY(QString pendingQrCode READ pendingQrCode WRITE setPendingQrCode NOTIFY pendingQrCodeChanged)

public:
    explicit TransferController(const std::shared_ptr<Settings> &settings,
                                const QSharedPointer<ServersModel> &serversModel,
                                ExportController *exportController,
                                QObject *parent = nullptr);
    ~TransferController() override;

    Q_INVOKABLE void generateNewQrCode();

    Q_INVOKABLE void stopScanner();
    Q_INVOKABLE void onTransferQrScanned(const QString &code);

    Q_INVOKABLE void setPendingQrCode(const QString &code) { m_pendingQrCode = code; emit pendingQrCodeChanged(); }
    QString pendingQrCode() const { return m_pendingQrCode; }

    // Waiting for config on receiver device
    Q_INVOKABLE void startWaitForConfig(ImportController *importController);
    Q_INVOKABLE void stopWaitForConfig();

    QString qrCodeUrl() const;

signals:
    void qrCodeUpdated();
    void scannerShouldStop();
    void pendingQrCodeChanged();

    void waitError(const QString &message);
    void configApplied();

    void postStarted();
    void postSucceeded();
    void postFailed(const QString &message);

private:
    void handleImportControllerDestroyed();
    QString buildQrPayloadJson(const QString &gatewayUrl, const QString &uuid, int version) const;
    QString getPremiumConfigToSend() const;
    QString m_pendingQrCode;
    QString getCurrentApiKey() const;

private:
    std::shared_ptr<Settings> m_settings;
    QSharedPointer<ServersModel> m_serversModel;
    ExportController *m_exportController { nullptr };
    ImportController *m_importController { nullptr };

    QString m_qrCodeUrl;
    QString m_currentUuid;
    QAtomicInt m_stopWaiting { 0 };
    QAtomicInt m_waitGeneration { 0 };
    QAtomicInt m_postInFlight { 0 };

    QFuture<void> m_waitFuture;
    QFuture<void> m_postFuture;
};

#endif // TRANSFERCONTROLLER_H 
