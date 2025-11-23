#ifndef LANGUAGEUICONTROLLER_H
#define LANGUAGEUICONTROLLER_H

#include <QObject>
#include <QLocale>

#include "settings.h"
#include "ui/models/languageModel.h"

class LanguageUiController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString currentLanguageName READ getCurrentLanguageName NOTIFY translationsUpdated)
    Q_PROPERTY(int currentLanguageIndex READ getCurrentLanguageIndex NOTIFY translationsUpdated)
    Q_PROPERTY(int lineHeightAppend READ getLineHeightAppend NOTIFY translationsUpdated)

public:
    explicit LanguageUiController(const std::shared_ptr<Settings> &settings,
                                  const QSharedPointer<LanguageModel> &languageModel,
                                  QObject *parent = nullptr);

public slots:
    void changeLanguage(const LanguageSettings::AvailableLanguageEnum language);
    int getCurrentLanguageIndex() const;
    int getLineHeightAppend() const;
    QString getCurrentLanguageName() const;
    QString getCurrentSiteUrl(const QString &path = "") const;
    QString getCurrentDocsUrl(const QString &path = "") const;
    LanguageSettings::AvailableLanguageEnum getSystemLanguageEnum() const;

signals:
    void updateTranslations(const QLocale &locale);
    void translationsUpdated();

private:
    QString getLocalLanguageName(const LanguageSettings::AvailableLanguageEnum language) const;
    QLocale languageEnumToLocale(const LanguageSettings::AvailableLanguageEnum language) const;

    std::shared_ptr<Settings> m_settings;
    QSharedPointer<LanguageModel> m_languageModel;
};

#endif // LANGUAGEUICONTROLLER_H

