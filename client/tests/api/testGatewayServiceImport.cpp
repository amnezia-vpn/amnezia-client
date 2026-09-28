#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcessEnvironment>
#include <QTest>
#include <QUuid>

#include "utils/testCoreController.h"
#include "core/controllers/api/subscriptionController.h"
#include "core/utils/constants/apiKeys.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/serverConfigUtils.h"

#include "amneziaApplication.h"
#include "secureQSettings.h"
#include "vpnConnection.h"

using namespace amnezia;

namespace
{
    constexpr const char *privateKeyPlaceholder = "$WIREGUARD_CLIENT_PRIVATE_KEY";

    QJsonObject awgContainer(const QString &clientPrivKeyValue)
    {
        QJsonObject lastConfig;
        lastConfig[configKey::hostName] = QStringLiteral("10.0.0.1");
        lastConfig[configKey::clientPrivKey] = clientPrivKeyValue;
        lastConfig[configKey::persistentKeepAlive] = QStringLiteral("25-35");

        QJsonObject awgConfig;
        awgConfig[configKey::lastConfig] = QString(QJsonDocument(lastConfig).toJson(QJsonDocument::Compact));
        awgConfig[configKey::port] = QStringLiteral("35333");

        QJsonObject container;
        container[configKey::container] = QStringLiteral("amnezia-awg");
        container[QString(configKey::awg)] = awgConfig;
        return container;
    }

    QJsonObject gatewayServerConfig(int configVersion = serverConfigUtils::ConfigSource::AmneziaGateway,
                                    int formatVersion = serverConfigUtils::currentConfigFormatVersion)
    {
        QJsonObject config;
        config[configKey::name] = QStringLiteral("Amnezia Premium");
        config[configKey::description] = QStringLiteral("Premium service");
        config[configKey::hostName] = QStringLiteral("gateway.example.org");
        config[configKey::configVersion] = configVersion;
        config[configKey::formatVersion] = formatVersion;
        config[configKey::defaultContainer] = QStringLiteral("amnezia-awg");
        config[configKey::containers] = QJsonArray { awgContainer(QString::fromLatin1(privateKeyPlaceholder)) };
        return config;
    }

    // Mirrors how the gateway ships a config in the `config` field of its v1/config response.
    QByteArray gatewayResponse(const QJsonObject &serverConfig, bool withServiceInfo = true)
    {
        const QByteArray payload = qCompress(QJsonDocument(serverConfig).toJson(QJsonDocument::Compact));

        QJsonObject response;
        response[apiDefs::key::config] =
                QStringLiteral("vpn://") + QString::fromUtf8(payload.toBase64(QByteArray::Base64UrlEncoding
                                                                              | QByteArray::OmitTrailingEquals));
        if (withServiceInfo) {
            QJsonObject serviceInfo;
            serviceInfo[apiDefs::key::isAdVisible] = true;
            serviceInfo[apiDefs::key::adHeader] = QStringLiteral("Promo header");
            response[apiDefs::key::serviceInfo] = serviceInfo;
        }
        return QJsonDocument(response).toJson(QJsonDocument::Compact);
    }

    // Populated from GitHub Secrets in CI; empty locally and on fork PRs.
    QString envValue(const char *name)
    {
        return QProcessEnvironment::systemEnvironment().value(QString::fromLatin1(name)).trimmed();
    }

    SubscriptionController::ProtocolData awgProtocolData()
    {
        SubscriptionController::ProtocolData data;
        data.wireGuardClientPrivKey = QStringLiteral("cHJpdmF0ZS1rZXktZm9yLXRlc3Q=");
        data.wireGuardClientPubKey = QStringLiteral("cHVibGljLWtleS1mb3ItdGVzdA==");
        return data;
    }
} // namespace

class TestGatewayServiceImport : public QObject
{
    Q_OBJECT

private:
    TestCoreController *m_coreController = nullptr;
    SecureQSettings *m_settings = nullptr;

    QJsonObject storedServer() const
    {
        if (m_coreController->m_serversRepository->serversCount() == 0) {
            return QJsonObject {};
        }
        const QString serverId = m_coreController->m_serversRepository->serverIdAt(0);
        const auto config = m_coreController->m_serversRepository->apiV2Config(serverId);
        return config ? config->toJson() : QJsonObject {};
    }

private slots:
    void initTestCase()
    {
        const QString testOrg = "AmneziaVPN-Test-" + QUuid::createUuid().toString();
        m_settings = new SecureQSettings(testOrg, "amnezia-client", nullptr, false);
        auto vpnConnection = QSharedPointer<VpnConnection>::create(nullptr, nullptr);
        m_coreController = new TestCoreController(vpnConnection, m_settings, nullptr, this);
    }

    void cleanupTestCase()
    {
        m_settings->clearSettings();
        delete m_coreController;
        delete m_settings;
    }

    void init()
    {
        m_settings->clearSettings();
        m_coreController->m_serversRepository->clearServers();
        m_coreController->m_serversModel->updateModel(QVector<ServerDescription>(), QString {});
    }

    void importsCompressedGatewayConfig()
    {
        const ErrorCode errorCode = m_coreController->m_subscriptionController->applyImportedServiceConfigForTest(
                QStringLiteral("NL"), QStringLiteral("amnezia-premium"), configKey::awg, awgProtocolData(),
                gatewayResponse(gatewayServerConfig()));

        QCOMPARE(errorCode, ErrorCode::NoError);
        QCOMPARE(m_coreController->m_serversRepository->serversCount(), 1);

        const QJsonObject server = storedServer();
        QCOMPARE(server.value(configKey::name).toString(), QStringLiteral("Amnezia Premium"));
        QCOMPARE(server.value(configKey::hostName).toString(), QStringLiteral("gateway.example.org"));
    }

    void substitutesWireguardPrivateKey()
    {
        const SubscriptionController::ProtocolData protocolData = awgProtocolData();
        const ErrorCode errorCode = m_coreController->m_subscriptionController->applyImportedServiceConfigForTest(
                QStringLiteral("NL"), QStringLiteral("amnezia-premium"), configKey::awg, protocolData,
                gatewayResponse(gatewayServerConfig()));

        QCOMPARE(errorCode, ErrorCode::NoError);

        const QJsonObject container = storedServer().value(configKey::containers).toArray().first().toObject();
        const QJsonObject awgConfig = container.value(QString(configKey::awg)).toObject();
        const QJsonObject lastConfig =
                QJsonDocument::fromJson(awgConfig.value(configKey::lastConfig).toString().toUtf8()).object();

        QCOMPARE(lastConfig.value(configKey::clientPrivKey).toString(), protocolData.wireGuardClientPrivKey);
        QVERIFY(!lastConfig.value(configKey::clientPrivKey).toString().contains(privateKeyPlaceholder));
    }

    void keepsAwg3RangeValuesAsStrings()
    {
        const ErrorCode errorCode = m_coreController->m_subscriptionController->applyImportedServiceConfigForTest(
                QStringLiteral("NL"), QStringLiteral("amnezia-premium"), configKey::awg, awgProtocolData(),
                gatewayResponse(gatewayServerConfig()));

        QCOMPARE(errorCode, ErrorCode::NoError);

        const QJsonObject container = storedServer().value(configKey::containers).toArray().first().toObject();
        const QJsonObject awgConfig = container.value(QString(configKey::awg)).toObject();
        const QJsonObject lastConfig =
                QJsonDocument::fromJson(awgConfig.value(configKey::lastConfig).toString().toUtf8()).object();

        // awg3 ships ranges such as "25-35"; they must survive import verbatim.
        QCOMPARE(lastConfig.value(configKey::persistentKeepAlive).toString(), QStringLiteral("25-35"));
    }

    void storesServiceMetadataFromResponse()
    {
        const ErrorCode errorCode = m_coreController->m_subscriptionController->applyImportedServiceConfigForTest(
                QStringLiteral("DE"), QStringLiteral("amnezia-premium"), configKey::awg, awgProtocolData(),
                gatewayResponse(gatewayServerConfig()));

        QCOMPARE(errorCode, ErrorCode::NoError);

        const QJsonObject apiConfig = storedServer().value(apiDefs::key::apiConfig).toObject();
        QCOMPARE(apiConfig.value(apiDefs::key::userCountryCode).toString(), QStringLiteral("DE"));
        QCOMPARE(apiConfig.value(apiDefs::key::serviceType).toString(), QStringLiteral("amnezia-premium"));
        QCOMPARE(apiConfig.value(apiDefs::key::serviceProtocol).toString(), QString(configKey::awg));
        const QJsonObject serviceInfo = apiConfig.value(apiDefs::key::serviceInfo).toObject();
        QCOMPARE(serviceInfo.value(apiDefs::key::isAdVisible).toBool(), true);
        QCOMPARE(serviceInfo.value(apiDefs::key::adHeader).toString(), QStringLiteral("Promo header"));
    }

    void rejectsEmptyConfigField()
    {
        QJsonObject response;
        response[apiDefs::key::config] = QString();

        const ErrorCode errorCode = m_coreController->m_subscriptionController->applyImportedServiceConfigForTest(
                QStringLiteral("NL"), QStringLiteral("amnezia-premium"), configKey::awg, awgProtocolData(),
                QJsonDocument(response).toJson(QJsonDocument::Compact));

        QCOMPARE(errorCode, ErrorCode::ApiConfigEmptyError);
        QCOMPARE(m_coreController->m_serversRepository->serversCount(), 0);
    }

    void rejectsConfigWithoutContainers()
    {
        QJsonObject config = gatewayServerConfig();
        config[configKey::containers] = QJsonArray {};

        const ErrorCode errorCode = m_coreController->m_subscriptionController->applyImportedServiceConfigForTest(
                QStringLiteral("NL"), QStringLiteral("amnezia-premium"), configKey::awg, awgProtocolData(),
                gatewayResponse(config));

        QCOMPARE(errorCode, ErrorCode::ApiConfigEmptyError);
        QCOMPARE(m_coreController->m_serversRepository->serversCount(), 0);
    }

    void rejectsUnsupportedFormatVersion()
    {
        const int unsupported = serverConfigUtils::currentConfigFormatVersion + 1;

        const ErrorCode errorCode = m_coreController->m_subscriptionController->applyImportedServiceConfigForTest(
                QStringLiteral("NL"), QStringLiteral("amnezia-premium"), configKey::awg, awgProtocolData(),
                gatewayResponse(gatewayServerConfig(serverConfigUtils::ConfigSource::AmneziaGateway, unsupported)));

        QCOMPARE(errorCode, ErrorCode::ConfigFormatVersionNotSupportedError);
        QCOMPARE(m_coreController->m_serversRepository->serversCount(), 0);
    }

    // --- live gateway import (requires GitHub Secrets) ---

    void importsServiceFromLiveGateway()
    {
        const QString endpoint = envValue("AMNEZIA_TEST_GATEWAY_ENDPOINT");
        const QString serviceType = envValue("AMNEZIA_TEST_SERVICE_TYPE");
        const QString serviceProtocol = envValue("AMNEZIA_TEST_SERVICE_PROTOCOL");
        const QString countryCode = envValue("AMNEZIA_TEST_COUNTRY_CODE");

        if (endpoint.isEmpty() || serviceType.isEmpty() || serviceProtocol.isEmpty()) {
            QSKIP("Gateway credentials are not configured, skipping live import test");
        }

        m_coreController->m_appSettingsRepository->setGatewayEndpoint(endpoint);

        const auto protocolData = SubscriptionController::generateProtocolData(serviceProtocol);
        SubscriptionController::CaptchaInfo captchaInfo;

        const ErrorCode errorCode = m_coreController->m_subscriptionController->importServiceFromGateway(
                countryCode, serviceType, serviceProtocol, protocolData, captchaInfo);

        if (errorCode == ErrorCode::ApiCaptchaRequiredError) {
            QSKIP("Gateway requested a captcha, skipping live import test");
        }

        QCOMPARE(errorCode, ErrorCode::NoError);
        QCOMPARE(m_coreController->m_serversRepository->serversCount(), 1);

        const QJsonObject server = storedServer();
        QVERIFY(!server.value(configKey::hostName).toString().isEmpty());
        QVERIFY(!server.value(configKey::containers).toArray().isEmpty());
        QCOMPARE(server.value(configKey::configVersion).toInt(), int(serverConfigUtils::ConfigSource::AmneziaGateway));

        const QJsonObject apiConfig = server.value(apiDefs::key::apiConfig).toObject();
        QCOMPARE(apiConfig.value(apiDefs::key::serviceType).toString(), serviceType);
        QCOMPARE(apiConfig.value(apiDefs::key::serviceProtocol).toString(), serviceProtocol);
    }

    void rejectsNonGatewayConfigSource()
    {
        const ErrorCode errorCode = m_coreController->m_subscriptionController->applyImportedServiceConfigForTest(
                QStringLiteral("NL"), QStringLiteral("amnezia-premium"), configKey::awg, awgProtocolData(),
                gatewayResponse(gatewayServerConfig(serverConfigUtils::ConfigSource::Telegram)));

        QCOMPARE(errorCode, ErrorCode::InternalError);
        QCOMPARE(m_coreController->m_serversRepository->serversCount(), 0);
    }
};

int main(int argc, char *argv[])
{
    // CoreController reaches the gateway through amnApp->networkManager(), so the
    // application instance has to really be an AmneziaApplication.
    AmneziaApplication app(argc, argv);
    TestGatewayServiceImport tc;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&tc, argc, argv);
}
#include "testGatewayServiceImport.moc"
