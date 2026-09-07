#ifndef LANGUAGEMODEL_H
#define LANGUAGEMODEL_H

#include <QAbstractListModel>
#include <QLocale>
#include <QQmlEngine>
#include <QVector>

namespace LanguageSettings
{
    Q_NAMESPACE
    enum class AvailableLanguageEnum {
        English,
        Russian,
        China_cn,
        Ukrainian,
        Persian,
        Arabic,
        Burmese,
        Urdu,
        Hindi,
        Spanish,
        Korean
    };
    Q_ENUM_NS(AvailableLanguageEnum)

    struct LanguageEntry
    {
        AvailableLanguageEnum language;
        QLocale::Language locale;
        const char *nativeName;
    };

    // Single source of truth for the supported languages. Entries must stay in
    // AvailableLanguageEnum order: the model exposes them as rows, and QML selects
    // a row by the enum value.
    const QVector<LanguageEntry> &availableLanguages();

    AvailableLanguageEnum localeToLanguage(const QLocale &locale);
    QLocale languageToLocale(const AvailableLanguageEnum language);
    QString nativeLanguageName(const AvailableLanguageEnum language);

    static void declareQmlAvailableLanguageEnum()
    {
        qmlRegisterUncreatableMetaObject(LanguageSettings::staticMetaObject, "AvailableLanguageEnum", 1, 0,
                                         "AvailableLanguageEnum", QString());
    }
}

struct LanguageModelData
{
    QString name;
    LanguageSettings::AvailableLanguageEnum index;
};

class LanguageModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        IndexRole
    };

    LanguageModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    QVector<LanguageModelData> m_availableLanguages;
};

#endif // LANGUAGEMODEL_H
