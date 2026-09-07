#include "languageModel.h"

namespace LanguageSettings
{
    AvailableLanguageEnum localeToLanguage(const QLocale &locale)
    {
        for (const auto &entry : availableLanguages) {
            if (entry.locale == locale.language()) {
                return entry.language;
            }
        }
        return AvailableLanguageEnum::English;
    }

    QLocale languageToLocale(const AvailableLanguageEnum language)
    {
        for (const auto &entry : availableLanguages) {
            if (entry.language == language) {
                return entry.locale;
            }
        }
        return QLocale::English;
    }

    QString nativeLanguageName(const AvailableLanguageEnum language)
    {
        for (const auto &entry : availableLanguages) {
            if (entry.language == language) {
                return QString::fromUtf8(entry.nativeName);
            }
        }
        return {};
    }
}

LanguageModel::LanguageModel(QObject *parent) : QAbstractListModel(parent)
{
    for (const auto &entry : LanguageSettings::availableLanguages) {
        m_availableLanguages.push_back(
                LanguageModelData { LanguageSettings::nativeLanguageName(entry.language), entry.language });
    }
}

int LanguageModel::rowCount(const QModelIndex &parent) const
{
    return static_cast<int>(m_availableLanguages.size());
}

QVariant LanguageModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_availableLanguages.size())) {
        return QVariant();
    }

    switch (role) {
    case NameRole: return m_availableLanguages[index.row()].name;
    case IndexRole: return static_cast<int>(m_availableLanguages[index.row()].index);
    }
    return QVariant();
}

QHash<int, QByteArray> LanguageModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "languageName";
    roles[IndexRole] = "languageIndex";
    return roles;
}
