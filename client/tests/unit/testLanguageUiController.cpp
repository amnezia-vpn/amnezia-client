#include <QSignalSpy>
#include <QTest>
#include <QUuid>

#include "secureQSettings.h"
#include "ui/controllers/languageUiController.h"
#include "ui/models/languageModel.h"
#include "utils/testCoreController.h"
#include "vpnConnection.h"

using namespace amnezia;

class TestLanguageUiController : public QObject
{
    Q_OBJECT

private:
    TestCoreController *m_coreController = nullptr;
    SecureQSettings *m_settings = nullptr;

    LanguageUiController *controller() const
    {
        return m_coreController->m_languageUiController;
    }

private slots:
    void initTestCase()
    {
        const QString organization = QStringLiteral("AmneziaVPN-Test-%1").arg(QUuid::createUuid().toString());

        m_settings = new SecureQSettings(organization, "amnezia-client", nullptr, false);

        auto vpnConnection = QSharedPointer<VpnConnection>::create(nullptr, nullptr);

        m_coreController = new TestCoreController(vpnConnection, m_settings, nullptr, this);
    }

    void cleanupTestCase()
    {
        delete m_coreController;
        m_coreController = nullptr;

        m_settings->clearSettings();
        delete m_settings;
        m_settings = nullptr;
    }

    void init()
    {
        m_settings->clearSettings();
        controller()->changeLanguage(LanguageSettings::AvailableLanguageEnum::English);
    }

    void testChangeLanguageEmitsSignals()
    {
        QSignalSpy updateSpy(controller(), &LanguageUiController::updateTranslations);
        QSignalSpy updatedSpy(controller(), &LanguageUiController::translationsUpdated);

        QVERIFY(updateSpy.isValid());
        QVERIFY(updatedSpy.isValid());

        controller()->changeLanguage(LanguageSettings::AvailableLanguageEnum::Russian);

        QCOMPARE(updateSpy.count(), 1);
        QCOMPARE(updatedSpy.count(), 1);

        const auto args = updateSpy.takeFirst();
        QCOMPARE(args.at(0).value<QLocale>().language(), QLocale::Russian);
    }

    void testCurrentLanguageIndex()
    {
        using Language = LanguageSettings::AvailableLanguageEnum;

        const QList<LanguageSettings::AvailableLanguageEnum> cases = {
            LanguageSettings::AvailableLanguageEnum::English,  LanguageSettings::AvailableLanguageEnum::Russian,
            LanguageSettings::AvailableLanguageEnum::China_cn, LanguageSettings::AvailableLanguageEnum::Ukrainian,
            LanguageSettings::AvailableLanguageEnum::Persian,  LanguageSettings::AvailableLanguageEnum::Arabic,
            LanguageSettings::AvailableLanguageEnum::Burmese,  LanguageSettings::AvailableLanguageEnum::Urdu,
            LanguageSettings::AvailableLanguageEnum::Hindi,    LanguageSettings::AvailableLanguageEnum::Spanish,
            LanguageSettings::AvailableLanguageEnum::Korean,   LanguageSettings::AvailableLanguageEnum::French,
            LanguageSettings::AvailableLanguageEnum::Thai,     LanguageSettings::AvailableLanguageEnum::Indonesian
        };

        for (const auto language : cases) {
            controller()->changeLanguage(language);

            QCOMPARE(controller()->getCurrentLanguageIndex(), static_cast<int>(language));
            QVERIFY(!controller()->getCurrentLanguageName().isEmpty());
        }
    }

    void testCurrentLanguageName()
    {
        using Language = LanguageSettings::AvailableLanguageEnum;

        const QList<QPair<Language, QString>> cases = { { Language::English, QStringLiteral("English") },
                                                        { Language::Russian, QStringLiteral("Русский") },
                                                        { Language::Ukrainian, QStringLiteral("Українська") },
                                                        { Language::China_cn, QStringLiteral("简体中文") },
                                                        { Language::Persian, QStringLiteral("فارسی") },
                                                        { Language::Arabic, QStringLiteral("العربية") },
                                                        { Language::Burmese, QStringLiteral("မြန်မာဘာသာ") },
                                                        { Language::Urdu, QStringLiteral("اُرْدُوْ") },
                                                        { Language::Hindi, QStringLiteral("हिन्दी") },
                                                        { Language::Korean, QStringLiteral("한국어") },
                                                        { Language::Spanish, QStringLiteral("Español") },
                                                        { Language::French, QStringLiteral("Français") },
                                                        { Language::Thai, QStringLiteral("ไทย") },
                                                        { Language::Indonesian, QStringLiteral("Bahasa Indonesia") } };

        for (const auto &[language, expectedName] : cases) {
            controller()->changeLanguage(language);
            QCOMPARE(controller()->getCurrentLanguageName(), expectedName);
        }
    }

    void testLineHeightAppend()
    {
        controller()->changeLanguage(LanguageSettings::AvailableLanguageEnum::Burmese);
        QCOMPARE(controller()->getLineHeightAppend(), 10);

        controller()->changeLanguage(LanguageSettings::AvailableLanguageEnum::English);
        QCOMPARE(controller()->getLineHeightAppend(), 0);
    }

    void testWebsiteUrls()
    {
        using Language = LanguageSettings::AvailableLanguageEnum;

        controller()->changeLanguage(Language::Russian);

        QCOMPARE(controller()->getCurrentSiteUrl(), QStringLiteral("https://storage.googleapis.com/amnezia/" "amnezia.org?utm_source=app&utm_campaign=amnezia_hello"));
        QCOMPARE(controller()->getCurrentSiteUrl(QStringLiteral("test")), QStringLiteral("https://storage.googleapis.com/amnezia/" "amnezia.org?utm_source=app&utm_campaign=amnezia_hello" "?m-path=/test"));

        controller()->changeLanguage(Language::English);

        QCOMPARE(controller()->getCurrentSiteUrl(), QStringLiteral("https://amnezia.org?utm_source=app" "&utm_campaign=amnezia_hello"));
        QCOMPARE(controller()->getCurrentSiteUrl(QStringLiteral("test")), QStringLiteral("https://amnezia.org?utm_source=app" "&utm_campaign=amnezia_hello/test"));
    }

    void testDocumentationUrls()
    {
        using Language = LanguageSettings::AvailableLanguageEnum;

        controller()->changeLanguage(Language::Russian);

        QCOMPARE(controller()->getCurrentDocsUrl(), QStringLiteral("https://storage.googleapis.com/amnezia/docs"));
        QCOMPARE(controller()->getCurrentDocsUrl(QStringLiteral("test")), QStringLiteral("https://storage.googleapis.com/amnezia/docs" "?m-path=/test"));

        controller()->changeLanguage(Language::English);

        QCOMPARE(controller()->getCurrentDocsUrl(), QStringLiteral("https://docs.amnezia.org"));
        QCOMPARE(controller()->getCurrentDocsUrl(QStringLiteral("test")), QStringLiteral("https://docs.amnezia.org/test"));
    }

    void testHostUrls()
    {
        using Language = LanguageSettings::AvailableLanguageEnum;

        controller()->changeLanguage(Language::Russian);

        QCOMPARE(controller()->getCurrentHostUrl(), QStringLiteral("https://storage.googleapis.com/amnezia/host"));
        QCOMPARE(controller()->getCurrentHostUrl(QStringLiteral("test")), QStringLiteral("https://storage.googleapis.com/amnezia/host" "?m-path=/test"));

        controller()->changeLanguage(Language::English);

        QCOMPARE(controller()->getCurrentHostUrl(), QStringLiteral("https://amnezia.host"));
        QCOMPARE(controller()->getCurrentHostUrl(QStringLiteral("test")), QStringLiteral("https://amnezia.host/test"));
    }

    void testSystemLanguageEnumIsSupported()
    {
        using Language = LanguageSettings::AvailableLanguageEnum;
        const auto language = controller()->getSystemLanguageEnum();

        const QList<Language> supportedLanguages = { Language::English,   Language::Russian,   Language::China_cn,
                                                     Language::Ukrainian, Language::Persian,   Language::Arabic,
                                                     Language::Burmese,   Language::Urdu,      Language::Hindi,
                                                     Language::Korean,    Language::Spanish,   Language::French,
                                                     Language::Thai,      Language::Indonesian };

        QVERIFY2(supportedLanguages.contains(language), "System language must map to a supported language");
    }
};

QTEST_MAIN(TestLanguageUiController)
#include "testLanguageUiController.moc"
