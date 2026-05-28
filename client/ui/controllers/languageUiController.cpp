#include "languageUiController.h"

LanguageUiController::LanguageUiController(SettingsController* settingsController,
                                           LanguageModel* languageModel,
                                           QObject *parent)
    : QObject(parent),
      m_settingsController(settingsController),
      m_languageModel(languageModel)
{
}

void LanguageUiController::onAppLanguageChanged(const QLocale &locale)
{
    emit updateTranslations(locale);
}

void LanguageUiController::changeLanguage(const LanguageSettings::AvailableLanguageEnum language)
{
    QLocale locale = languageEnumToLocale(language);
    m_settingsController->setAppLanguage(locale);
}

int LanguageUiController::getCurrentLanguageIndex() const
{
    auto locale = m_settingsController->getAppLanguage();
    switch (locale.language()) {
    case QLocale::English: return static_cast<int>(LanguageSettings::AvailableLanguageEnum::English); break;
    case QLocale::Russian: return static_cast<int>(LanguageSettings::AvailableLanguageEnum::Russian); break;
    case QLocale::Chinese: return static_cast<int>(LanguageSettings::AvailableLanguageEnum::China_cn); break;
    case QLocale::Ukrainian: return static_cast<int>(LanguageSettings::AvailableLanguageEnum::Ukrainian); break;
    case QLocale::Persian: return static_cast<int>(LanguageSettings::AvailableLanguageEnum::Persian); break;
    case QLocale::Arabic: return static_cast<int>(LanguageSettings::AvailableLanguageEnum::Arabic); break;
    case QLocale::Burmese: return static_cast<int>(LanguageSettings::AvailableLanguageEnum::Burmese); break;
    case QLocale::Urdu: return static_cast<int>(LanguageSettings::AvailableLanguageEnum::Urdu); break;
    case QLocale::Hindi: return static_cast<int>(LanguageSettings::AvailableLanguageEnum::Hindi); break;
    default: return static_cast<int>(LanguageSettings::AvailableLanguageEnum::English); break;
    }
}

int LanguageUiController::getLineHeightAppend() const
{
    auto locale = m_settingsController->getAppLanguage();
    switch (locale.language()) {
    case QLocale::Burmese: return 10; break;
    default: return 0; break;
    }
}

QString LanguageUiController::getCurrentLanguageName() const
{
    int index = getCurrentLanguageIndex();
    return getLocalLanguageName(static_cast<LanguageSettings::AvailableLanguageEnum>(index));
}

LanguageSettings::AvailableLanguageEnum LanguageUiController::getSystemLanguageEnum() const
{
    QLocale locale = QLocale::system();
    switch (locale.language()) {
    case QLocale::Russian: return LanguageSettings::AvailableLanguageEnum::Russian;
    case QLocale::Chinese: return LanguageSettings::AvailableLanguageEnum::China_cn;
    case QLocale::Ukrainian: return LanguageSettings::AvailableLanguageEnum::Ukrainian;
    case QLocale::Persian: return LanguageSettings::AvailableLanguageEnum::Persian;
    case QLocale::Arabic: return LanguageSettings::AvailableLanguageEnum::Arabic;
    case QLocale::Burmese: return LanguageSettings::AvailableLanguageEnum::Burmese;
    case QLocale::Urdu: return LanguageSettings::AvailableLanguageEnum::Urdu;
    case QLocale::Hindi: return LanguageSettings::AvailableLanguageEnum::Hindi;
    case QLocale::English: return LanguageSettings::AvailableLanguageEnum::English;
    default: return LanguageSettings::AvailableLanguageEnum::English;
    }
}

QString LanguageUiController::getCurrentSiteUrl(const QString &path) const
{
    auto locale = m_settingsController->getAppLanguage();
    if (locale.language() == QLocale::Russian) {
        return "https://storage.googleapis.com/amnezia/amnezia.org" + (path.isEmpty() ? "" : (QString("?m-path=/%1").arg(path)));
    }
    return QString("https://amnezia.org") + (path.isEmpty() ? "" : (QString("/%1").arg(path)));
}

QString LanguageUiController::getCurrentDocsUrl(const QString &path) const
{
    auto locale = m_settingsController->getAppLanguage();
    if (locale.language() == QLocale::Russian) {
        return "https://storage.googleapis.com/amnezia/docs" + (path.isEmpty() ? "" : (QString("?m-path=/%1").arg(path)));
    }
    return QString("https://docs.amnezia.org") + (path.isEmpty() ? "" : (QString("/%1").arg(path)));
}

QString LanguageUiController::getLocalLanguageName(const LanguageSettings::AvailableLanguageEnum language) const
{
    QString strLanguage("");
    switch (language) {
    case LanguageSettings::AvailableLanguageEnum::English: strLanguage = "English"; break;
    case LanguageSettings::AvailableLanguageEnum::Russian: strLanguage = "Русский"; break;
    case LanguageSettings::AvailableLanguageEnum::Ukrainian: strLanguage = "Українська"; break;
    case LanguageSettings::AvailableLanguageEnum::China_cn: strLanguage = "\347\256\200\344\275\223\344\270\255\346\226\207"; break;
    case LanguageSettings::AvailableLanguageEnum::Persian: strLanguage = "فارسی"; break;
    case LanguageSettings::AvailableLanguageEnum::Arabic: strLanguage = "العربية"; break;
    case LanguageSettings::AvailableLanguageEnum::Burmese: strLanguage = "မြန်မာဘာသာ"; break;
    case LanguageSettings::AvailableLanguageEnum::Urdu: strLanguage = "اُرْدُوْ"; break;
    case LanguageSettings::AvailableLanguageEnum::Hindi: strLanguage = "हिन्दी"; break;
    default: break;
    }

    return strLanguage;
}

QLocale LanguageUiController::languageEnumToLocale(const LanguageSettings::AvailableLanguageEnum language) const
{
    switch (language) {
    case LanguageSettings::AvailableLanguageEnum::English: return QLocale::English;
    case LanguageSettings::AvailableLanguageEnum::Russian: return QLocale::Russian;
    case LanguageSettings::AvailableLanguageEnum::China_cn: return QLocale::Chinese;
    case LanguageSettings::AvailableLanguageEnum::Ukrainian: return QLocale::Ukrainian;
    case LanguageSettings::AvailableLanguageEnum::Persian: return QLocale::Persian;
    case LanguageSettings::AvailableLanguageEnum::Arabic: return QLocale::Arabic;
    case LanguageSettings::AvailableLanguageEnum::Burmese: return QLocale::Burmese;
    case LanguageSettings::AvailableLanguageEnum::Urdu: return QLocale::Urdu;
    case LanguageSettings::AvailableLanguageEnum::Hindi: return QLocale::Hindi;
    default: return QLocale::English;
    }
}

