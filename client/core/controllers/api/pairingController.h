#ifndef PAIRINGCONTROLLER_H
#define PAIRINGCONTROLLER_H

#include <QJsonArray>
#include <QJsonObject>
#include <QString>

#include "core/utils/errorCodes.h"

class SecureAppSettingsRepository;

/**
 * Core API for QR pairing against Amnezia gateway (POST /v1/generate_qr, /v1/scan_qr).
 * Phase 1: transport via GatewayController, error mapping incl. gateway `http_status` wrapper and OpenAPI-style bodies.
 */
class PairingController
{
public:
    struct QrPairingConfigPayload
    {
        QString config;
        QJsonObject serviceInfo;
        QJsonArray supportedProtocols;
    };

    explicit PairingController(SecureAppSettingsRepository *appSettingsRepository);

    int pairingLongPollTimeoutMsecs() const;

    QJsonObject buildGenerateQrPayload(const QString &qrUuid) const;
    QJsonObject buildScanQrPayload(const QString &qrUuid, const QString &vpnConfig, const QJsonObject &serviceInfo,
                                   const QJsonArray &supportedProtocols, const QString &apiKey, const QString &serviceType,
                                   const QString &userCountryCode) const;

    static amnezia::ErrorCode parseGenerateQrResponseBody(const QByteArray &responseBody, QrPairingConfigPayload &outPayload);
    static amnezia::ErrorCode parseScanQrResponseBody(const QByteArray &responseBody, QString *outOptionalDisplayName = nullptr);

    static amnezia::ErrorCode validatePairingScanFields(const QString &qrUuid, const QString &vpnConfig, const QString &apiKey,
                                                        const QString &serviceType, const QString &userCountryCode);

    static QJsonArray gatewayStringMetadataArray(const QString &value);

private:
    SecureAppSettingsRepository *m_appSettingsRepository;
};

#endif // PAIRINGCONTROLLER_H
