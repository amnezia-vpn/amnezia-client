#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcessEnvironment>
#include <QSignalSpy>
#include <QTest>
#include <QUuid>

#include "core/controllers/coreController.h"
#include "core/models/serverConfig.h"
#include "secureQSettings.h"
#include "vpnConnection.h"

using namespace amnezia;

class TestUiLanguageModelAndController : public QObject
{
    Q_OBJECT

private:
    CoreController *m_coreController;
    SecureQSettings *m_settings;

private slots:
    void initTestCase()
    {
        QString testOrg = "AmneziaVPN-Test-" + QUuid::createUuid().toString();
        m_settings = new SecureQSettings(testOrg, "amnezia-client", nullptr, false);

        auto vpnConnection = QSharedPointer<VpnConnection>::create(nullptr, nullptr);

        m_coreController = new CoreController(vpnConnection, m_settings, nullptr, this);
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
        if (m_coreController->m_serversModel) {
            m_coreController->m_serversModel->updateModel(QVector<ServerConfig>(), -1, false);
        }
    }

    void testChangeLanguage()
    {
        QVERIFY(m_coreController->m_languageModel->rowCount() > 0, "Language model should not be empty");

        QSignalSpy updateTranslationsSpy(m_coreController->m_languageUiController, &LanguageUiController::updateTranslations);
        QSignalSpy translationsUpdatedSpy(m_coreController->m_languageUiController, &LanguageUiController::translationsUpdated);

        m_coreController->m_languageUiController->changeLanguage(LanguageSettings::AvailableLanguageEnum::China_cn);
        QVERIFY(updateTranslationsSpy.count() == 1, "updateTranslations signal should be emitted");
        QVERIFY(translationsUpdatedSpy.count() == 1, "translationsUpdated signal should be emitted");

        m_coreController->m_languageUiController->changeLanguage(LanguageSettings::AvailableLanguageEnum::English);
        QVERIFY(updateTranslationsSpy.count() == 2, "updateTranslations signal should be emitted");
        QVERIFY(translationsUpdatedSpy.count() == 2, "translationsUpdated signal should be emitted");
    }

    void testUrl()
    {
        m_coreController->m_languageUiController->changeLanguage(LanguageSettings::AvailableLanguageEnum::Russian);
        QString siteRU = m_coreController->m_languageUiController->getCurrentSiteUrl("test_path");
        QString docsRU = m_coreController->m_languageUiController->getCurrentDocsUrl("test_path");

        m_coreController->m_languageUiController->changeLanguage(LanguageSettings::AvailableLanguageEnum::English);
        QString siteEN = m_coreController->m_languageUiController->getCurrentSiteUrl("test_path");
        QString docsEN = m_coreController->m_languageUiController->getCurrentDocsUrl("test_path");

        QVERIFY(siteRU != siteEN, "site url's should not be same");
        QVERIFY(docsRU != docsEN, "docs url's should not be same");
    }

    void testLineHeight()
    {
        m_coreController->m_languageUiController->changeLanguage(LanguageSettings::AvailableLanguageEnum::Burmese);
        QVERIFY(m_coreController->m_languageUiController->getLineHeightAppend() == 10, "line height should be 10");

        m_coreController->m_languageUiController->changeLanguage(LanguageSettings::AvailableLanguageEnum::English);
        QVERIFY(m_coreController->m_languageUiController->getLineHeightAppend() == 0, "line height should be 0");
    }
};

QTEST_MAIN(TestUiLanguageModelAndController)
#include "testUiLanguageModelAndController.moc"
