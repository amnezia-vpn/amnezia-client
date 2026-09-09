#ifndef LANGUAGEMODEL_H
#define LANGUAGEMODEL_H

#include <array>

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

    // Single source of truth for the supported languages. The order is part of the
    // contract, not a convention: the model exposes these as rows and QML selects a
    // row by enum value. The static_assert below turns a wrong order into a build error.
    inline constexpr std::array<LanguageEntry, 11> availableLanguages { {
            { AvailableLanguageEnum::English, QLocale::English, "English" },
            { AvailableLanguageEnum::Russian, QLocale::Russian, "Русский" },
            { AvailableLanguageEnum::China_cn, QLocale::Chinese, "\347\256\200\344\275\223\344\270\255\346\226\207" },
            { AvailableLanguageEnum::Ukrainian, QLocale::Ukrainian, "Українська" },
            { AvailableLanguageEnum::Persian, QLocale::Persian, "فارسی" },
            { AvailableLanguageEnum::Arabic, QLocale::Arabic, "العربية" },
            { AvailableLanguageEnum::Burmese, QLocale::Burmese, "မြန်မာဘာသာ" },
            { AvailableLanguageEnum::Urdu, QLocale::Urdu, "اُرْدُوْ" },
            { AvailableLanguageEnum::Hindi, QLocale::Hindi, "हिन्दी" },
            { AvailableLanguageEnum::Spanish, QLocale::Spanish, "Español" },
            { AvailableLanguageEnum::Korean, QLocale::Korean, "한국어" },
    } };

    constexpr bool areLanguagesInEnumOrder()
    {
        for (std::size_t i = 0; i < availableLanguages.size(); ++i) {
            if (static_cast<std::size_t>(availableLanguages[i].language) != i) {
                return false;
            }
        }
        return true;
    }

    static_assert(areLanguagesInEnumOrder(),
                  "availableLanguages must stay in AvailableLanguageEnum order: the model exposes it as rows "
                  "and QML selects a row by enum value");

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
