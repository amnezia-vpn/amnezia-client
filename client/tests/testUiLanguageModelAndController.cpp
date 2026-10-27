#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcessEnvironment>
#include <QSignalSpy>
#include <QUuid>
#include <QTest>

#include "utils/testCoreController.h"
#include "core/models/serverDescription.h"
#include "secureQSettings.h"
#include "vpnConnection.h"

using namespace amnezia;

class TestUiLanguageModelAndController : public QObject
{
    Q_OBJECT

private:
    TestCoreController *m_coreController;
    SecureQSettings *m_settings;

private slots:
    void initTestCase()
    {
        QString testOrg = "AmneziaVPN-Test-" + QUuid::createUuid().toString();
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
        if (m_coreController->m_serversModel) {
            m_coreController->m_serversModel->updateModel(QVector<ServerDescription>(), QString{});
        }
    }

    void testChangeLanguage()
    {
        QVERIFY2(m_coreController->m_languageModel->rowCount() > 0, "Language model should not be empty");

        QSignalSpy updateTranslationsSpy(m_coreController->m_languageUiController, &LanguageUiController::updateTranslations);
        QSignalSpy translationsUpdatedSpy(m_coreController->m_languageUiController, &LanguageUiController::translationsUpdated);

        m_coreController->m_languageUiController->changeLanguage(LanguageSettings::AvailableLanguageEnum::China_cn);
        QVERIFY2(updateTranslationsSpy.count() == 1, "updateTranslations signal should be emitted");
        QVERIFY2(translationsUpdatedSpy.count() == 1, "translationsUpdated signal should be emitted");

        m_coreController->m_languageUiController->changeLanguage(LanguageSettings::AvailableLanguageEnum::English);
        QVERIFY2(updateTranslationsSpy.count() == 2, "updateTranslations signal should be emitted");
        QVERIFY2(translationsUpdatedSpy.count() == 2, "translationsUpdated signal should be emitted");
    }

    void testUrl()
    {
        m_coreController->m_languageUiController->changeLanguage(LanguageSettings::AvailableLanguageEnum::Russian);
        QString siteRU = m_coreController->m_languageUiController->getCurrentSiteUrl("test_path");
        QString docsRU = m_coreController->m_languageUiController->getCurrentDocsUrl("test_path");

        m_coreController->m_languageUiController->changeLanguage(LanguageSettings::AvailableLanguageEnum::English);
        QString siteEN = m_coreController->m_languageUiController->getCurrentSiteUrl("test_path");
        QString docsEN = m_coreController->m_languageUiController->getCurrentDocsUrl("test_path");

        QVERIFY2(siteRU != siteEN, "site url's should not be same");
        QVERIFY2(docsRU != docsEN, "docs url's should not be same");
    }

    void testLineHeight()
    {
        m_coreController->m_languageUiController->changeLanguage(LanguageSettings::AvailableLanguageEnum::Burmese);
        QVERIFY2(m_coreController->m_languageUiController->getLineHeightAppend() == 10, "line height should be 10");

        m_coreController->m_languageUiController->changeLanguage(LanguageSettings::AvailableLanguageEnum::English);
        QVERIFY2(m_coreController->m_languageUiController->getLineHeightAppend() == 0, "line height should be 0");
    }

    void testEveryLanguageHasNativeName()
    {
        LanguageModel *model = m_coreController->m_languageModel;
        LanguageUiController *controller = m_coreController->m_languageUiController;

        const int count = model->rowCount();
        QVERIFY2(count > 0, "Language model should not be empty");

        for (int i = 0; i < count; i++) {
            const QModelIndex idx = model->index(i, 0);

            const QString modelName = model->data(idx, LanguageModel::NameRole).toString();
            QVERIFY2(!modelName.isEmpty(),
                     qPrintable(QString("Language at row %1 has no native name in the model").arg(i)));

            const int languageIndex = model->data(idx, LanguageModel::IndexRole).toInt();
            QVERIFY2(languageIndex == i,
                     qPrintable(QString("Language at row %1 reports enum index %2").arg(i).arg(languageIndex)));

            const auto language = static_cast<LanguageSettings::AvailableLanguageEnum>(languageIndex);
            controller->changeLanguage(language);

            const QString currentName = controller->getCurrentLanguageName();
            QVERIFY2(!currentName.isEmpty(),
                     qPrintable(QString("Language '%1' has no native name in the controller").arg(languageIndex)));

            QVERIFY2(modelName == currentName,
                     qPrintable(QString("Native name mismatch for enum index %1: model '%2' vs controller '%3'")
                                        .arg(languageIndex)
                                        .arg(modelName, currentName)));

            QVERIFY2(controller->getCurrentLanguageIndex() == languageIndex,
                     qPrintable(QString("Language enum index %1 does not round-trip").arg(languageIndex)));
        }
    }
};

QTEST_MAIN(TestUiLanguageModelAndController)
#include "testUiLanguageModelAndController.moc"
