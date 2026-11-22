#include <QJsonArray>
#include <QJsonObject>
#include <QTest>

#include "core/utils/serialization/serialization.h"

using namespace amnezia;

class TestShadowSocksSerialization : public QObject
{
    Q_OBJECT

private slots:
    void deserializesUserInfo_data()
    {
        QTest::addColumn<QString>("uri");
        QTest::addColumn<QString>("method");
        QTest::addColumn<QString>("password");

        QTest::newRow("base64 user info") << "ss://YWVzLTI1Ni1nY206dGVzdC1wYXNzd29yZA@ss.example.com:8388#base64"
                                          << "aes-256-gcm" << "test-password";
        QTest::newRow("plain user info")
                << "ss://2022-blake3-aes-128-gcm:dGVzdC1rZXktMDEyMzQ1Njc4OWFiY2RlZg%3D%3D@ss.example.com:8388#plain"
                << "2022-blake3-aes-128-gcm" << "dGVzdC1rZXktMDEyMzQ1Njc4OWFiY2RlZg==";
    }

    void deserializesUserInfo()
    {
        QFETCH(QString, uri);
        QFETCH(QString, method);
        QFETCH(QString, password);

        QString alias;
        QString errorMessage;
        const QJsonObject config = serialization::ss::Deserialize(uri, &alias, &errorMessage);

        QVERIFY2(errorMessage.isEmpty(), qPrintable(errorMessage));
        const QJsonObject outbound = config.value("outbounds").toArray().first().toObject();
        const QJsonObject server = outbound.value("settings").toObject().value("servers").toArray().first().toObject();
        QCOMPARE(server.value("address").toString(), QString("ss.example.com"));
        QCOMPARE(server.value("port").toInt(), 8388);
        QCOMPARE(server.value("method").toString(), method);
        QCOMPARE(server.value("password").toString(), password);
    }
};

QTEST_APPLESS_MAIN(TestShadowSocksSerialization)

#include "testShadowSocksSerialization.moc"
