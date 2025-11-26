#ifndef LANGUAGEUICONTROLLER_H
#define LANGUAGEUICONTROLLER_H

#include <QObject>
#include <QLocale>

#include "core/repositories/qAppSettingsRepository.h"
#include "ui/models/languageModel.h"

class LanguageUiController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString currentLanguageName READ getCurrentLanguageName NOTIFY translationsUpdated)
    Q_PROPERTY(int currentLanguageIndex READ getCurrentLanguageIndex NOTIFY translationsUpdated)
    Q_PROPERTY(int lineHeightAppend READ getLineHeightAppend NOTIFY translationsUpdated)

public:
    explicit LanguageUiController(const QSharedPointer<QAppSettingsRepository> &appSettingsRepository,
                                  const QSharedPointer<LanguageModel> &languageModel,
                                  QObject *parent = nullptr);

public slots:
    void changeLanguage(const LanguageSettings::AvailableLanguageEnum language);
    void onAppLanguageChanged(const QLocale &locale);
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

    QSharedPointer<QAppSettingsRepository> m_appSettingsRepository;
    QSharedPointer<LanguageModel> m_languageModel;
};

#endif // LANGUAGEUICONTROLLER_H

