#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTest>

#include "core/controllers/api/pairingController.h"
#include "ui/controllers/api/pairingUiController.h"
#include "core/utils/constants/apiKeys.h"

using namespace amnezia;

class TestPairingParsers : public QObject
{
    Q_OBJECT

private slots:
    void generateQr_success_extractsConfigAndMeta()
    {
        PairingController::QrPairingConfigPayload out;
        QJsonObject o;
        o[apiDefs::key::config] = QStringLiteral("vpn://dummy");
        o[apiDefs::key::serviceInfo] = QJsonObject { { QStringLiteral("is_ad_visible"), false } };
        o[apiDefs::key::supportedProtocols] = QJsonArray { QStringLiteral("awg") };
        const QByteArray body = QJsonDocument(o).toJson();

        QCOMPARE(PairingController::parseGenerateQrResponseBody(body, out), ErrorCode::NoError);
        QCOMPARE(out.config, QStringLiteral("vpn://dummy"));
        QCOMPARE(out.supportedProtocols.size(), 1);
    }

    void generateQr_http408()
    {
        PairingController::QrPairingConfigPayload out;
        QJsonObject o;
        o[QStringLiteral("http_status")] = 408;
        o[QStringLiteral("message")] = QStringLiteral("Request Timeout");
        const QByteArray body = QJsonDocument(o).toJson();

        QCOMPARE(PairingController::parseGenerateQrResponseBody(body, out), ErrorCode::ApiConfigTimeoutError);
        QVERIFY(out.config.isEmpty());
    }

    void generateQr_http429()
    {
        PairingController::QrPairingConfigPayload out;
        QJsonObject o;
        o[QStringLiteral("http_status")] = 429;
        o[QStringLiteral("message")] = QStringLiteral("Too Many Requests");
        const QByteArray body = QJsonDocument(o).toJson();

        QCOMPARE(PairingController::parseGenerateQrResponseBody(body, out), ErrorCode::ApiPairingRateLimitedError);
    }

    void scanQr_messageOk()
    {
        QJsonObject o;
        o[QStringLiteral("message")] = QStringLiteral("OK");
        const QByteArray body = QJsonDocument(o).toJson();

        QCOMPARE(PairingController::parseScanQrResponseBody(body), ErrorCode::NoError);
    }

    void scanQr_messageOk_extractsDeviceName()
    {
        QJsonObject o;
        o[QStringLiteral("message")] = QStringLiteral("OK");
        o[QStringLiteral("device_name")] = QStringLiteral("TestPhone");
        const QByteArray body = QJsonDocument(o).toJson();
        QString name;
        QCOMPARE(PairingController::parseScanQrResponseBody(body, &name), ErrorCode::NoError);
        QCOMPARE(name, QStringLiteral("TestPhone"));
    }

    void scanQr_deviceLimitMessage()
    {
        QJsonObject o;
        o[QStringLiteral("message")] = QStringLiteral("Device limit reached for subscription");
        const QByteArray body = QJsonDocument(o).toJson();

        QCOMPARE(PairingController::parseScanQrResponseBody(body), ErrorCode::ApiConfigLimitError);
    }

    void scanQr_http403()
    {
        QJsonObject o;
        o[QStringLiteral("http_status")] = 403;
        const QByteArray body = QJsonDocument(o).toJson();

        QCOMPARE(PairingController::parseScanQrResponseBody(body), ErrorCode::ApiPairingForbiddenError);
    }

    void scanQr_http409()
    {
        QJsonObject o;
        o[QStringLiteral("http_status")] = 409;
        const QByteArray body = QJsonDocument(o).toJson();

        QCOMPARE(PairingController::parseScanQrResponseBody(body), ErrorCode::ApiPairingConflictError);
    }

    void scanQr_notFoundMessage()
    {
        QJsonObject o;
        o[QStringLiteral("message")] = QStringLiteral("Session not found");
        const QByteArray body = QJsonDocument(o).toJson();

        QCOMPARE(PairingController::parseScanQrResponseBody(body), ErrorCode::ApiNotFoundError);
    }

    void validateScanFields_oversizedVpnKey()
    {
        QString vpnKey;
        vpnKey.fill(QLatin1Char('x'), 256 * 1024 + 1);
        QCOMPARE(PairingController::validatePairingScanFields(QStringLiteral("ab"), vpnKey, QStringLiteral("k"),
                                                              QStringLiteral("amnezia-premium"), QStringLiteral("ru")),
                 ErrorCode::ApiPairingPayloadTooLargeError);
    }

    void validateScanFields_uuidTooLong()
    {
        QString uuid(200, QLatin1Char('a'));
        QCOMPARE(PairingController::validatePairingScanFields(uuid, QStringLiteral("vpn://a"), QStringLiteral("k"),
                                                              QStringLiteral("amnezia-premium"), QStringLiteral("ru")),
                 ErrorCode::ApiConfigEmptyError);
    }

    void validateScanFields_missingServiceType()
    {
        QCOMPARE(PairingController::validatePairingScanFields(QStringLiteral("ab"), QStringLiteral("vpn://x"),
                                                              QStringLiteral("k"), QString(),
                                                              QStringLiteral("ru")),
                 ErrorCode::ApiPairingMissingMetadataError);
    }

    void pairingUi_applyScanned_extractsUuid_emitsSignal()
    {
        PairingUiController ctl(nullptr, nullptr, nullptr, nullptr);
        QSignalSpy spy(&ctl, &PairingUiController::pairingUuidFromScan);
        const QString u = QStringLiteral("123e4567-e89b-12d3-a456-426614174000");
        QVERIFY(ctl.applyScannedTextAsPairingUuid(QStringLiteral("prefix ") + u + QStringLiteral(" suffix")));
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.first().first().toString(), u);
    }

    void pairingUi_applyScanned_rejectsVpnKey()
    {
        PairingUiController ctl(nullptr, nullptr, nullptr, nullptr);
        QSignalSpy spy(&ctl, &PairingUiController::pairingUuidFromScan);
        QVERIFY(!ctl.applyScannedTextAsPairingUuid(QStringLiteral("vpn://AAAA")));
        QCOMPARE(spy.count(), 0);
    }
};

QTEST_MAIN(TestPairingParsers)
#include "testPairingParsers.moc"
