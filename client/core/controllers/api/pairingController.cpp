#include "pairingController.h"

#include <QJsonDocument>
#include <QSysInfo>

#include "core/controllers/gatewayController.h"
#include "core/repositories/secureAppSettingsRepository.h"
#include "core/utils/api/apiUtils.h"
#include "core/utils/constants/apiConstants.h"
#include "core/utils/constants/apiKeys.h"
#include "version.h"

using namespace amnezia;

namespace
{
constexpr auto kGenerateQrEndpoint = "%1api/v1/generate_qr";
constexpr auto kScanQrEndpoint = "%1api/v1/scan_qr";
constexpr qsizetype kPairingMaxQrUuidChars = 128;
constexpr qsizetype kPairingMaxVpnConfigChars = 256 * 1024;
constexpr qsizetype kPairingMaxApiKeyChars = 8192;

bool isLocalGatewayHost(const QString &gatewayUrl)
{
    return gatewayUrl.contains(QStringLiteral("127.0.0.1"), Qt::CaseInsensitive)
            || gatewayUrl.contains(QStringLiteral("localhost"), Qt::CaseInsensitive)
            || gatewayUrl.contains(QStringLiteral("[::1]"), Qt::CaseInsensitive)
            || gatewayUrl.contains(QStringLiteral("::1"), Qt::CaseInsensitive);
}

ErrorCode applyGatewayOrOpenApiGenerateError(const QJsonObject &obj, PairingController::QrPairingConfigPayload &outPayload)
{
    ErrorCode apiStatus = apiUtils::errorCodeFromGatewayJsonHttpStatus(obj);
    if (apiStatus != ErrorCode::NoError) {
        return apiStatus;
    }

    const QString config = obj.value(apiDefs::key::config).toString();
    if (!config.isEmpty()) {
        outPayload.config = config;
        outPayload.serviceInfo = obj.value(apiDefs::key::serviceInfo).toObject();
        outPayload.supportedProtocols = obj.value(apiDefs::key::supportedProtocols).toArray();
        return ErrorCode::NoError;
    }

    if (obj.contains(QStringLiteral("detail"))) {
        return ErrorCode::ApiConfigEmptyError;
    }

    const QString msg = obj.value(QStringLiteral("message")).toString();
    if (msg.contains(QStringLiteral("timeout"), Qt::CaseInsensitive)) {
        return ErrorCode::ApiConfigTimeoutError;
    }
    if (msg.contains(QStringLiteral("Too Many"), Qt::CaseInsensitive)) {
        return ErrorCode::ApiPairingRateLimitedError;
    }
    if (msg.contains(QStringLiteral("Unavailable"), Qt::CaseInsensitive)) {
        return ErrorCode::ApiPairingServiceUnavailableError;
    }
    if (!msg.isEmpty()) {
        return ErrorCode::ApiConfigDownloadError;
    }

    return ErrorCode::ApiConfigEmptyError;
}

ErrorCode applyGatewayOrOpenApiScanError(const QJsonObject &obj)
{
    ErrorCode apiStatus = apiUtils::errorCodeFromGatewayJsonHttpStatus(obj);
    if (apiStatus != ErrorCode::NoError) {
        return apiStatus;
    }

    if (obj.value(QStringLiteral("message")).toString() == QLatin1String("OK")) {
        return ErrorCode::NoError;
    }

    if (obj.contains(QStringLiteral("detail"))) {
        return ErrorCode::ApiPairingForbiddenError;
    }

    const QString msg = obj.value(QStringLiteral("message")).toString();
    if (msg.contains(QStringLiteral("not found"), Qt::CaseInsensitive) || msg.contains(QStringLiteral("expired"), Qt::CaseInsensitive)) {
        return ErrorCode::ApiNotFoundError;
    }
    if (msg.contains(QStringLiteral("Conflict"), Qt::CaseInsensitive) || msg.contains(QStringLiteral("already"), Qt::CaseInsensitive)) {
        return ErrorCode::ApiPairingConflictError;
    }
    if (msg.contains(QStringLiteral("Too Many"), Qt::CaseInsensitive)) {
        return ErrorCode::ApiPairingRateLimitedError;
    }
    if (msg.contains(QStringLiteral("Unavailable"), Qt::CaseInsensitive)) {
        return ErrorCode::ApiPairingServiceUnavailableError;
    }
    if (!msg.isEmpty()) {
        return ErrorCode::ApiConfigDownloadError;
    }

    return ErrorCode::ApiConfigEmptyError;
}

ErrorCode interpretGenerateQrJson(const QJsonObject &obj, PairingController::QrPairingConfigPayload &outPayload)
{
    return applyGatewayOrOpenApiGenerateError(obj, outPayload);
}

ErrorCode interpretScanQrJson(const QJsonObject &obj)
{
    return applyGatewayOrOpenApiScanError(obj);
}
} // namespace

ErrorCode PairingController::parseGenerateQrResponseBody(const QByteArray &responseBody, QrPairingConfigPayload &outPayload)
{
    outPayload = QrPairingConfigPayload {};
    const QJsonObject obj = QJsonDocument::fromJson(responseBody).object();
    return interpretGenerateQrJson(obj, outPayload);
}

ErrorCode PairingController::parseScanQrResponseBody(const QByteArray &responseBody)
{
    const QJsonObject obj = QJsonDocument::fromJson(responseBody).object();
    return interpretScanQrJson(obj);
}

ErrorCode PairingController::validatePairingScanFields(const QString &qrUuid, const QString &vpnConfig, const QString &apiKey)
{
    if (qrUuid.size() > kPairingMaxQrUuidChars) {
        return ErrorCode::ApiConfigEmptyError;
    }
    if (vpnConfig.size() > kPairingMaxVpnConfigChars) {
        return ErrorCode::ApiPairingPayloadTooLargeError;
    }
    if (apiKey.size() > kPairingMaxApiKeyChars) {
        return ErrorCode::ApiPairingPayloadTooLargeError;
    }
    return ErrorCode::NoError;
}

PairingController::PairingController(SecureAppSettingsRepository *appSettingsRepository)
    : m_appSettingsRepository(appSettingsRepository)
{
}

int PairingController::pairingLongPollTimeoutMsecs() const
{
    const QString endpoint = m_appSettingsRepository->getGatewayEndpoint();
    if (isLocalGatewayHost(endpoint)) {
        return 120 * 1000;
    }
    return 30 * 1000;
}

QJsonObject PairingController::buildGenerateQrPayload(const QString &qrUuid) const
{
    QJsonObject o;
    o[apiDefs::key::qrUuid] = qrUuid;
    o[apiDefs::key::installationUuid] = m_appSettingsRepository->getInstallationUuid(true);
    o[apiDefs::key::appVersion] = QString(APP_VERSION);
    o[apiDefs::key::osVersion] = QSysInfo::productType();
    return o;
}

QJsonObject PairingController::buildScanQrPayload(const QString &qrUuid, const QString &vpnConfig, const QJsonObject &serviceInfo,
                                                  const QJsonArray &supportedProtocols, const QString &apiKey) const
{
    QJsonObject auth;
    auth[apiDefs::key::apiKey] = apiKey;

    QJsonObject o;
    o[apiDefs::key::qrUuid] = qrUuid;
    o[apiDefs::key::config] = vpnConfig;
    o[apiDefs::key::serviceInfo] = serviceInfo;
    o[apiDefs::key::supportedProtocols] = supportedProtocols;
    o[apiDefs::key::authData] = auth;
    o[apiDefs::key::installationUuid] = m_appSettingsRepository->getInstallationUuid(true);
    o[apiDefs::key::appVersion] = QString(APP_VERSION);
    o[apiDefs::key::osVersion] = QSysInfo::productType();
    return o;
}

ErrorCode PairingController::startPairing(const QString &qrUuid, QrPairingConfigPayload &outPayload)
{
    outPayload = QrPairingConfigPayload {};
    if (qrUuid.isEmpty()) {
        return ErrorCode::ApiConfigEmptyError;
    }

    GatewayController gatewayController(m_appSettingsRepository->getGatewayEndpoint(), m_appSettingsRepository->isDevGatewayEnv(),
                                        pairingLongPollTimeoutMsecs(), m_appSettingsRepository->isStrictKillSwitchEnabled());

    QByteArray responseBody;
    const ErrorCode transportError = gatewayController.post(QString::fromLatin1(kGenerateQrEndpoint), buildGenerateQrPayload(qrUuid), responseBody);
    if (transportError != ErrorCode::NoError) {
        return transportError;
    }

    const QJsonObject obj = QJsonDocument::fromJson(responseBody).object();
    return interpretGenerateQrJson(obj, outPayload);
}

ErrorCode PairingController::completePairing(const QString &qrUuid, const QString &vpnConfig, const QJsonObject &serviceInfo,
                                             const QJsonArray &supportedProtocols, const QString &apiKey)
{
    if (qrUuid.isEmpty() || vpnConfig.isEmpty() || apiKey.isEmpty()) {
        return ErrorCode::ApiConfigEmptyError;
    }

    const ErrorCode fieldErr = validatePairingScanFields(qrUuid, vpnConfig, apiKey);
    if (fieldErr != ErrorCode::NoError) {
        return fieldErr;
    }

    GatewayController gatewayController(m_appSettingsRepository->getGatewayEndpoint(), m_appSettingsRepository->isDevGatewayEnv(),
                                        apiDefs::requestTimeoutMsecs, m_appSettingsRepository->isStrictKillSwitchEnabled());

    QByteArray responseBody;
    const ErrorCode transportError =
            gatewayController.post(QString::fromLatin1(kScanQrEndpoint),
                                   buildScanQrPayload(qrUuid, vpnConfig, serviceInfo, supportedProtocols, apiKey), responseBody);
    if (transportError != ErrorCode::NoError) {
        return transportError;
    }

    const QJsonObject obj = QJsonDocument::fromJson(responseBody).object();
    return interpretScanQrJson(obj);
}
