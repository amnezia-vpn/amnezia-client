#include <QCoreApplication>
#include <QDebug>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QHostAddress>
#include <QHostInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslConfiguration>
#include <QSslError>
#include <QTest>
#include <QUrl>

#include "networkUtilities.h"
#include "QBlockCipher.h"
#include "QRsa.h"

#include <openssl/evp.h>
#include <openssl/rsa.h>

struct TransportResult {
    QString name;
    bool success = false;
    int elapsedMs = 0;
    int responseSize = 0;
    QString error;
    QByteArray responseBody;
};

struct TestConfig {
    QString httpEndpoint;
    struct DnsEntry {
        QString name;
        NetworkUtilities::DnsTransport type;
        QString server;
        QString domain;
        quint16 port;
        QString dohPath;
    };
    QList<DnsEntry> dnsTransports;
    int timeoutMs = 15000;
};

static TestConfig loadConfig(const QString &path)
{
    TestConfig cfg;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open config:" << path;
        return cfg;
    }
    QJsonObject json = QJsonDocument::fromJson(f.readAll()).object();

    if (json.contains("http")) {
        cfg.httpEndpoint = json["http"].toObject().value("endpoint").toString();
    }
    cfg.timeoutMs = json.value("timeout_ms").toInt(15000);

    if (json.contains("dns_transports")) {
        for (const auto &v : json["dns_transports"].toArray()) {
            QJsonObject obj = v.toObject();
            TestConfig::DnsEntry e;
            e.server = obj.value("server").toString();
            e.domain = obj.value("domain").toString();
            e.port = static_cast<quint16>(obj.value("port").toInt(5353));
            e.dohPath = obj.value("path").toString("/dns-query");

            QString t = obj.value("type").toString().toLower();
            if (t == "udp") {
                e.type = NetworkUtilities::DnsTransport::Udp;
                e.name = "UDP";
            } else if (t == "tcp") {
                e.type = NetworkUtilities::DnsTransport::Tcp;
                e.name = "TCP";
            } else if (t == "dot" || t == "tls") {
                e.type = NetworkUtilities::DnsTransport::Tls;
                e.name = "DoT";
            } else if (t == "doh" || t == "https") {
                e.type = NetworkUtilities::DnsTransport::Https;
                e.name = "DoH";
            } else if (t == "doq" || t == "quic") {
                e.type = NetworkUtilities::DnsTransport::Quic;
                e.name = "DoQ";
            } else {
                continue;
            }
            cfg.dnsTransports.append(e);
        }
    }
    return cfg;
}

static QString resolveHost(const QString &host)
{
    QHostAddress addr(host);
    if (!addr.isNull()) return host;
    QHostInfo info = QHostInfo::fromName(host);
    if (!info.addresses().isEmpty())
        return info.addresses().first().toString();
    return host;
}

// Replicate the RSA+AES encryption from GatewayController::prepareRequest
struct EncryptedPayload {
    QByteArray body;
    QByteArray key;
    QByteArray iv;
    QByteArray salt;
    bool ok = false;
    QString error;
};

static EncryptedPayload encryptPayload(const QJsonObject &apiPayload, const QByteArray &rsaPubKeyPem)
{
    EncryptedPayload result;

    QSimpleCrypto::QBlockCipher blockCipher;
    result.key = blockCipher.generatePrivateSalt(32);
    result.iv = blockCipher.generatePrivateSalt(32);
    result.salt = blockCipher.generatePrivateSalt(8);

    QJsonObject keyPayload;
    keyPayload["aes_key"] = QString(result.key.toBase64());
    keyPayload["aes_iv"] = QString(result.iv.toBase64());
    keyPayload["aes_salt"] = QString(result.salt.toBase64());

    try {
        QSimpleCrypto::QRsa rsa;
        QByteArray pemData = rsaPubKeyPem;
        pemData.replace("\\n", "\n");
        EVP_PKEY *pubKey = rsa.getPublicKeyFromByteArray(pemData);
        if (!pubKey) {
            result.error = "Failed to load RSA public key";
            return result;
        }

        QByteArray encKeyPayload = rsa.encrypt(QJsonDocument(keyPayload).toJson(), pubKey, RSA_PKCS1_PADDING);
        EVP_PKEY_free(pubKey);

        QByteArray encApiPayload = blockCipher.encryptAesBlockCipher(
            QJsonDocument(apiPayload).toJson(), result.key, result.iv, "", result.salt);

        QJsonObject requestBody;
        requestBody["key_payload"] = QString(encKeyPayload.toBase64());
        requestBody["api_payload"] = QString(encApiPayload.toBase64());

        result.body = QJsonDocument(requestBody).toJson();
        result.ok = true;
    } catch (const std::exception &ex) {
        result.error = QString("Encryption failed: %1").arg(ex.what());
    } catch (...) {
        result.error = "Encryption failed: unknown error";
    }
    return result;
}

static QByteArray decryptResponse(const QByteArray &encrypted, const QByteArray &key,
                                   const QByteArray &iv, const QByteArray &salt)
{
    try {
        QSimpleCrypto::QBlockCipher blockCipher;
        return blockCipher.decryptAesBlockCipher(encrypted, key, iv, "", salt);
    } catch (...) {
        return QByteArray();
    }
}

class TransportTest : public QObject
{
    Q_OBJECT

private:
    TestConfig m_config;
    QByteArray m_rsaKey;
    bool m_hasRsaKey = false;
    QList<TransportResult> m_results;

    void logResult(const TransportResult &r) {
        QString status = r.success ? "OK" : "FAIL";
        qDebug().noquote() << QString("[%1] %2 | %3ms | %4 bytes | %5")
            .arg(status, -4)
            .arg(r.name, -20)
            .arg(r.elapsedMs, 5)
            .arg(r.responseSize, 6)
            .arg(r.error.isEmpty() ? "---" : r.error);
    }

    TransportResult doHttpTransport(const QString &endpoint, const QByteArray &payload) {
        TransportResult r;
        r.name = "HTTP";
        QElapsedTimer timer;
        timer.start();

        QNetworkAccessManager nam;
        QNetworkRequest request(QUrl(endpoint));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        request.setTransferTimeout(m_config.timeoutMs);

        QNetworkReply *reply = nam.post(request, payload);

        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        r.elapsedMs = static_cast<int>(timer.elapsed());

        if (reply->error() != QNetworkReply::NoError) {
            r.error = QString("HTTP %1: %2")
                .arg(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt())
                .arg(reply->errorString());
            r.responseBody = reply->readAll();
            r.responseSize = r.responseBody.size();
        } else {
            r.responseBody = reply->readAll();
            r.responseSize = r.responseBody.size();
            r.success = !r.responseBody.isEmpty();
            if (!r.success) r.error = "Empty response";
        }
        reply->deleteLater();
        return r;
    }

    TransportResult doDnsTransport(const TestConfig::DnsEntry &entry, const QByteArray &payload,
                                   const QString &resolvedIp) {
        TransportResult r;
        r.name = QString("DNS-%1").arg(entry.name);
        QElapsedTimer timer;
        timer.start();

        bool needsHostname = (entry.type == NetworkUtilities::DnsTransport::Https ||
                              entry.type == NetworkUtilities::DnsTransport::Tls);
        QString serverAddr = needsHostname ? entry.server : resolvedIp;

        r.responseBody = NetworkUtilities::sendViaDnsTunnel(
            payload, "services", entry.domain,
            serverAddr, entry.type, entry.port,
            m_config.timeoutMs, entry.dohPath);

        r.elapsedMs = static_cast<int>(timer.elapsed());
        r.responseSize = r.responseBody.size();
        r.success = !r.responseBody.isEmpty();
        if (!r.success) r.error = "Empty/no response";
        return r;
    }

private slots:
    void initTestCase()
    {
        QString configPath = QCoreApplication::applicationDirPath() + "/gateway.json";
        if (!QFile::exists(configPath)) {
            configPath = QString(CLIENT_SOURCE_DIR) + "/gateway.json";
        }
        qDebug() << "Loading config from:" << configPath;
        m_config = loadConfig(configPath);

        QVERIFY2(!m_config.httpEndpoint.isEmpty(), "gateway.json: http endpoint missing");
        QVERIFY2(!m_config.dnsTransports.isEmpty(), "gateway.json: no dns_transports configured");

        qDebug() << "HTTP endpoint:" << m_config.httpEndpoint;
        qDebug() << "DNS transports:" << m_config.dnsTransports.size();
        qDebug() << "Timeout:" << m_config.timeoutMs << "ms";

        QByteArray prodKey(PROD_AGW_PUBLIC_KEY);
        QByteArray devKey(DEV_AGW_PUBLIC_KEY);
        if (!prodKey.isEmpty()) {
            m_rsaKey = prodKey;
            m_hasRsaKey = true;
            qDebug() << "Using PROD_AGW_PUBLIC_KEY for E2E tests";
        } else if (!devKey.isEmpty()) {
            m_rsaKey = devKey;
            m_hasRsaKey = true;
            qDebug() << "Using DEV_AGW_PUBLIC_KEY for E2E tests";
        } else {
            qWarning() << "No RSA public key found -- E2E tests will be SKIPPED";
        }
    }

    // ========== Transport-level tests (raw payload, no encryption) ==========

    void test_transport_http()
    {
        QByteArray payload = R"({"test":true})";
        TransportResult r = doHttpTransport(m_config.httpEndpoint, payload);
        m_results.append(r);
        logResult(r);
        QVERIFY2(r.success || r.responseSize > 0,
                 qPrintable(QString("HTTP transport failed: %1").arg(r.error)));
    }

    void test_transport_dns_data()
    {
        QTest::addColumn<int>("transportIndex");
        for (int i = 0; i < m_config.dnsTransports.size(); ++i) {
            const auto &e = m_config.dnsTransports[i];
            if (e.type == NetworkUtilities::DnsTransport::Quic) continue;
            QTest::newRow(qPrintable(e.name)) << i;
        }
    }

    void test_transport_dns()
    {
        QFETCH(int, transportIndex);
        const auto &entry = m_config.dnsTransports[transportIndex];
        QString resolvedIp = resolveHost(entry.server);
        qDebug() << "Server:" << entry.server << "-> IP:" << resolvedIp
                 << "Port:" << entry.port;

        QByteArray payload = R"({"test":true})";
        TransportResult r = doDnsTransport(entry, payload, resolvedIp);
        m_results.append(r);
        logResult(r);

        if (!r.success) {
            qWarning() << "DNS" << entry.name << "transport failed (server may be down):" << r.error;
        }
    }

    // ========== E2E tests (RSA+AES encryption, full round-trip) ==========

    void test_e2e_http()
    {
        if (!m_hasRsaKey) QSKIP("No RSA key -- skipping E2E");

        QJsonObject apiPayload;
        apiPayload["protocol"] = "any";
        EncryptedPayload enc = encryptPayload(apiPayload, m_rsaKey);
        QVERIFY2(enc.ok, qPrintable(enc.error));

        TransportResult r = doHttpTransport(m_config.httpEndpoint, enc.body);
        r.name = "E2E-HTTP";

        if (r.success) {
            QByteArray decrypted = decryptResponse(r.responseBody, enc.key, enc.iv, enc.salt);
            if (!decrypted.isEmpty()) {
                r.responseBody = decrypted;
                r.responseSize = decrypted.size();
                qDebug() << "Decrypted response:" << decrypted.left(200);
            } else {
                r.error = "Decryption failed (raw body size: " + QString::number(r.responseBody.size()) + ")";
                r.success = false;
            }
        }

        m_results.append(r);
        logResult(r);
        QVERIFY2(r.success, qPrintable(QString("E2E HTTP failed: %1").arg(r.error)));
    }

    void test_e2e_dns_data()
    {
        QTest::addColumn<int>("transportIndex");
        for (int i = 0; i < m_config.dnsTransports.size(); ++i) {
            const auto &e = m_config.dnsTransports[i];
            if (e.type == NetworkUtilities::DnsTransport::Quic) continue;
            QTest::newRow(qPrintable(QString("E2E-%1").arg(e.name))) << i;
        }
    }

    void test_e2e_dns()
    {
        if (!m_hasRsaKey) QSKIP("No RSA key -- skipping E2E");

        QFETCH(int, transportIndex);
        const auto &entry = m_config.dnsTransports[transportIndex];
        QString resolvedIp = resolveHost(entry.server);
        qDebug() << "E2E via" << entry.name << "server:" << entry.server
                 << "-> IP:" << resolvedIp << "port:" << entry.port;

        QJsonObject apiPayload;
        apiPayload["protocol"] = "any";
        EncryptedPayload enc = encryptPayload(apiPayload, m_rsaKey);
        QVERIFY2(enc.ok, qPrintable(enc.error));

        TransportResult r = doDnsTransport(entry, enc.body, resolvedIp);
        r.name = QString("E2E-%1").arg(entry.name);

        if (r.success) {
            QByteArray decrypted = decryptResponse(r.responseBody, enc.key, enc.iv, enc.salt);
            if (!decrypted.isEmpty()) {
                r.responseBody = decrypted;
                r.responseSize = decrypted.size();
                qDebug() << "Decrypted response:" << decrypted.left(200);
            } else {
                r.error = "Decryption failed (raw body size: " + QString::number(r.responseBody.size()) + ")";
                r.success = false;
            }
        }

        m_results.append(r);
        logResult(r);

        if (!r.success) {
            qWarning() << "E2E DNS" << entry.name << "failed:" << r.error;
        }
    }

    // ========== Summary ==========

    void cleanupTestCase()
    {
        qDebug() << "";
        qDebug() << "============================================================";
        qDebug() << "                    TRANSPORT TEST SUMMARY";
        qDebug() << "============================================================";
        qDebug().noquote() << QString(" %-4s | %-20s | %5s | %6s | %s")
            .arg("", "Transport", "ms", "bytes", "Error");
        qDebug() << "------------------------------------------------------------";

        int passed = 0, failed = 0;
        for (const auto &r : m_results) {
            logResult(r);
            if (r.success) ++passed; else ++failed;
        }

        qDebug() << "------------------------------------------------------------";
        qDebug().noquote() << QString("Total: %1 passed, %2 failed, %3 total")
            .arg(passed).arg(failed).arg(m_results.size());
        qDebug() << "============================================================";
    }
};

QTEST_MAIN(TransportTest)
#include "tst_transports.moc"
