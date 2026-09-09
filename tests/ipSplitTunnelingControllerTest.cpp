#include <QtTest>
#include <QSettings>
#include <QTemporaryDir>

#include "core/controllers/ipSplitTunnelingController.h"
#include "core/repositories/secureAppSettingsRepository.h"
#include "secureQSettings.h"

class IpSplitTunnelingControllerTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();

    void importPreservesCidr();
    void manualAddPreservesCidr();
    void exportAndImportPreservesCidr();
    void normalizesUrlPathButKeepsDomain();

private:
    QTemporaryDir m_settingsDir;
    SecureQSettings *m_settings = nullptr;
    SecureAppSettingsRepository *m_repository = nullptr;
    IpSplitTunnelingController *m_controller = nullptr;
};

void IpSplitTunnelingControllerTest::initTestCase()
{
    QVERIFY(m_settingsDir.isValid());
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settingsDir.path());
}

void IpSplitTunnelingControllerTest::init()
{
    m_settings = new SecureQSettings(
        QStringLiteral("AmneziaVPN"), QStringLiteral("IpSplitTunnelingControllerTest"), this, false);
    m_settings->clearSettings();
    m_repository = new SecureAppSettingsRepository(m_settings, this);
    m_controller = new IpSplitTunnelingController(m_repository, this);
}

void IpSplitTunnelingControllerTest::cleanup()
{
    delete m_controller;
    m_controller = nullptr;
    delete m_repository;
    m_repository = nullptr;
    delete m_settings;
    m_settings = nullptr;
}

void IpSplitTunnelingControllerTest::importPreservesCidr()
{
    const QByteArray json = R"json([
        {"hostname":"1.192.0.0/10","ip":""},
        {"hostname":"2.16.0.0/13","ips":[]}
    ])json";
    QString errorMessage;

    QVERIFY2(m_controller->importSitesFromJson(json, true, errorMessage), qPrintable(errorMessage));

    const auto sites = m_controller->getCurrentSites();
    QCOMPARE(sites.size(), 2);
    QCOMPARE(sites.at(0).first, QStringLiteral("1.192.0.0/10"));
    QCOMPARE(sites.at(1).first, QStringLiteral("2.16.0.0/13"));
}

void IpSplitTunnelingControllerTest::manualAddPreservesCidr()
{
    QVERIFY(m_controller->addSite(QStringLiteral("1.192.0.0/10")));

    const auto sites = m_controller->getCurrentSites();
    QCOMPARE(sites.size(), 1);
    QCOMPARE(sites.first().first, QStringLiteral("1.192.0.0/10"));
}

void IpSplitTunnelingControllerTest::exportAndImportPreservesCidr()
{
    const QByteArray json = R"json([
        {"hostname":"1.192.0.0/10","ip":""},
        {"hostname":"example.com","ip":"1.1.1.1"}
    ])json";
    QString errorMessage;
    QVERIFY2(m_controller->importSitesFromJson(json, true, errorMessage), qPrintable(errorMessage));

    const QByteArray exported = m_controller->exportSitesToJson();
    m_controller->removeSites();
    QVERIFY2(m_controller->importSitesFromJson(exported, true, errorMessage), qPrintable(errorMessage));

    const auto sites = m_controller->getCurrentSites();
    QCOMPARE(sites.size(), 2);
    QCOMPARE(sites.at(0).first, QStringLiteral("1.192.0.0/10"));
    QCOMPARE(sites.at(1).first, QStringLiteral("example.com"));
}

void IpSplitTunnelingControllerTest::normalizesUrlPathButKeepsDomain()
{
    QVERIFY(m_controller->addSite(QStringLiteral("https://example.com/path")));
    QVERIFY(m_controller->addSite(QStringLiteral("plain.example.com")));

    const auto sites = m_controller->getCurrentSites();
    QCOMPARE(sites.size(), 2);
    QCOMPARE(sites.at(0).first, QStringLiteral("example.com"));
    QCOMPARE(sites.at(1).first, QStringLiteral("plain.example.com"));
}

QTEST_APPLESS_MAIN(IpSplitTunnelingControllerTest)
