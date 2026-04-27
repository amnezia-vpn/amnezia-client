#include <QtTest>

#include "core/scripts_registry.h"

namespace
{
QStringList *g_messages = nullptr;

void testMessageHandler(QtMsgType, const QMessageLogContext &, const QString &message)
{
    if (g_messages) {
        g_messages->append(message);
    }
}
} // namespace

class ScriptsRegistryTest : public QObject
{
    Q_OBJECT

private slots:
    void optionalStartupLookupReturnsEmptyWithoutWarning();
    void strictLookupStillWarnsWhenMissing();
};

void ScriptsRegistryTest::optionalStartupLookupReturnsEmptyWithoutWarning()
{
    QStringList messages;
    g_messages = &messages;
    const auto previousHandler = qInstallMessageHandler(testMessageHandler);

    const QString script = amnezia::scriptDataIfExists(amnezia::ProtocolScriptType::container_startup, amnezia::DockerContainer::Dns);

    qInstallMessageHandler(previousHandler);
    g_messages = nullptr;

    QCOMPARE(script, QString());
    QVERIFY2(!messages.join('\n').contains(":/server_scripts/dns/start.sh"),
             "optional startup lookups must not emit missing-script warnings");
}

void ScriptsRegistryTest::strictLookupStillWarnsWhenMissing()
{
    QStringList messages;
    g_messages = &messages;
    const auto previousHandler = qInstallMessageHandler(testMessageHandler);

    const QString script = amnezia::scriptData(amnezia::ProtocolScriptType::container_startup, amnezia::DockerContainer::Dns);

    qInstallMessageHandler(previousHandler);
    g_messages = nullptr;

    QCOMPARE(script, QString());
    QVERIFY2(messages.join('\n').contains(":/server_scripts/dns/start.sh"),
             "strict startup lookups must keep the existing warning behavior");
}

QTEST_APPLESS_MAIN(ScriptsRegistryTest)

#include "tst_scripts_registry.moc"
