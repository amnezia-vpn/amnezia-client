#ifndef TRANSFERCONTROLLER_H
#define TRANSFERCONTROLLER_H

#include <QObject>
#include <QScopedPointer>
#include <QJsonObject>
#include <QJsonDocument>
#include <QUuid>

class Settings;
class ServersModel;
class ExportController;
class ImportController;

class TransferController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString qrCodeUrl READ qrCodeUrl NOTIFY qrCodeUpdated)
    Q_PROPERTY(QString pendingQrCode READ pendingQrCode WRITE setPendingQrCode NOTIFY pendingQrCodeChanged)
    // Debug / manual transfer support (temporary): expose UUID and payload JSON used to generate the QR code.
    Q_PROPERTY(QString currentUuid READ currentUuid NOTIFY currentUuidChanged)
    Q_PROPERTY(QString currentPayload READ currentPayload NOTIFY currentPayloadChanged)

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
    QString currentUuid() const { return m_currentUuid; }
    QString currentPayload() const { return m_currentPayload; }

signals:
    void qrCodeUpdated();
    void scannerShouldStop();
    void pendingQrCodeChanged();
    void currentUuidChanged();
    void currentPayloadChanged();

    void waitError(const QString &message);
    void configApplied();

    void postStarted();
    void postSucceeded();
    void postFailed(const QString &message);

private slots:
    void handleImportControllerDestroyed();

private:
    QString buildQrPayloadJson(const QString &gatewayUrl, const QString &uuid) const;
    //QString getPremiumConfigToSend() const;
    QString m_pendingQrCode;
    QString getCurrentApiKey(QString *vpnKeyOut = nullptr) const;
    std::shared_ptr<Settings> m_settings;
    QSharedPointer<ServersModel> m_serversModel;
    ExportController *m_exportController { nullptr };
    ImportController *m_importController { nullptr };

    QString m_qrCodeUrl;
    QString m_currentUuid;
    QString m_currentPayload;
};

#endif // TRANSFERCONTROLLER_H 
