#include "pairingController.h"

#include <QJsonDocument>
#include <QSysInfo>
#include "core/repositories/secureAppSettingsRepository.h"
#include "core/utils/api/apiUtils.h"
#include "core/utils/constants/apiConstants.h"
#include "core/utils/constants/apiKeys.h"
#include "version.h"

using namespace amnezia;

namespace
{
constexpr qsizetype kPairingMaxQrUuidChars = 128;
constexpr qsizetype kPairingMaxVpnConfigChars = 256 * 1024;
constexpr qsizetype kPairingMaxApiKeyChars = 8192;
constexpr qsizetype kPairingMaxServiceTypeChars = 64;
constexpr qsizetype kPairingMaxUserCountryCodeChars = 32;

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
    const QString msgProbe = obj.value(QStringLiteral("message")).toString();
    if (msgProbe.contains(QStringLiteral("limit"), Qt::CaseInsensitive)
        && (msgProbe.contains(QStringLiteral("device"), Qt::CaseInsensitive)
            || msgProbe.contains(QStringLiteral("maximum"), Qt::CaseInsensitive)
            || msgProbe.contains(QStringLiteral("max"), Qt::CaseInsensitive))) {
        return ErrorCode::ApiConfigLimitError;
    }

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

QJsonArray PairingController::gatewayStringMetadataArray(const QString &value)
{
    QJsonArray arr;
    const QString trimmed = value.trimmed();
    if (!trimmed.isEmpty()) {
        arr.append(trimmed);
    }
    return arr;
}

ErrorCode PairingController::parseGenerateQrResponseBody(const QByteArray &responseBody, QrPairingConfigPayload &outPayload)
{
    outPayload = QrPairingConfigPayload {};
    const QJsonObject obj = QJsonDocument::fromJson(responseBody).object();
    return interpretGenerateQrJson(obj, outPayload);
}

ErrorCode PairingController::parseScanQrResponseBody(const QByteArray &responseBody, QString *outOptionalDisplayName)
{
    if (outOptionalDisplayName) {
        outOptionalDisplayName->clear();
    }
    const QJsonObject obj = QJsonDocument::fromJson(responseBody).object();
    const ErrorCode err = interpretScanQrJson(obj);
    if (err != ErrorCode::NoError) {
        return err;
    }
    if (outOptionalDisplayName) {
        const QString deviceName = obj.value(QStringLiteral("device_name")).toString().trimmed();
        if (!deviceName.isEmpty()) {
            *outOptionalDisplayName = deviceName;
        }
    }
    return ErrorCode::NoError;
}

ErrorCode PairingController::validatePairingScanFields(const QString &qrUuid, const QString &vpnConfig, const QString &apiKey,
                                                       const QString &serviceType, const QString &userCountryCode)
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
    const QString st = serviceType.trimmed();
    const QString cc = userCountryCode.trimmed();
    if (st.isEmpty() || cc.isEmpty()) {
        return ErrorCode::ApiPairingMissingMetadataError;
    }
    if (st.size() > kPairingMaxServiceTypeChars || cc.size() > kPairingMaxUserCountryCodeChars) {
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
    return 60 * 1000;
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
                                                    const QJsonArray &supportedProtocols, const QString &apiKey,
                                                    const QString &serviceType, const QString &userCountryCode) const
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
    o[apiDefs::key::serviceType] = gatewayStringMetadataArray(serviceType);
    o[apiDefs::key::userCountryCode] = gatewayStringMetadataArray(userCountryCode);
    return o;
}
