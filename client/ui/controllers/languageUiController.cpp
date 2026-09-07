#include "languageUiController.h"

LanguageUiController::LanguageUiController(SettingsController *settingsController, LanguageModel *languageModel, QObject *parent)
    : QObject(parent), m_settingsController(settingsController), m_languageModel(languageModel)
{
}

void LanguageUiController::onAppLanguageChanged(const QLocale &locale)
{
    emit updateTranslations(locale);
}

void LanguageUiController::changeLanguage(const LanguageSettings::AvailableLanguageEnum language)
{
    QLocale locale = LanguageSettings::languageToLocale(language);
    m_settingsController->setAppLanguage(locale);
}

int LanguageUiController::getCurrentLanguageIndex() const
{
    return static_cast<int>(LanguageSettings::localeToLanguage(m_settingsController->getAppLanguage()));
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
    return LanguageSettings::nativeLanguageName(static_cast<LanguageSettings::AvailableLanguageEnum>(index));
}

LanguageSettings::AvailableLanguageEnum LanguageUiController::getSystemLanguageEnum() const
{
    return LanguageSettings::localeToLanguage(QLocale::system());
}

QString LanguageUiController::getCurrentSiteUrl(const QString &path) const
{
    auto locale = m_settingsController->getAppLanguage();
    if (locale.language() == QLocale::Russian) {
        return "https://storage.googleapis.com/amnezia/amnezia.org?utm_source=app&utm_campaign=amnezia_hello" + (path.isEmpty() ? "" : (QString("?m-path=/%1").arg(path)));
    }
    return QString("https://amnezia.org?utm_source=app&utm_campaign=amnezia_hello") + (path.isEmpty() ? "" : (QString("/%1").arg(path)));
}

QString LanguageUiController::getCurrentDocsUrl(const QString &path) const
{
    auto locale = m_settingsController->getAppLanguage();
    if (locale.language() == QLocale::Russian) {
        return "https://storage.googleapis.com/amnezia/docs" + (path.isEmpty() ? "" : (QString("?m-path=/%1").arg(path)));
    }
    return QString("https://docs.amnezia.org") + (path.isEmpty() ? "" : (QString("/%1").arg(path)));
}

QString LanguageUiController::getCurrentHostUrl(const QString &path) const
{
    auto locale = m_settingsController->getAppLanguage();
    if (locale.language() == QLocale::Russian) {
        return "https://storage.googleapis.com/amnezia/host" + (path.isEmpty() ? "" : (QString("?m-path=/%1").arg(path)));
    }
    return QString("https://amnezia.host") + (path.isEmpty() ? "" : (QString("/%1").arg(path)));
}
