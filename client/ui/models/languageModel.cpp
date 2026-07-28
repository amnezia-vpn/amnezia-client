#include "languageModel.h"

LanguageModel::LanguageModel(QObject *parent) : QAbstractListModel(parent)
{
    QMetaEnum metaEnum = QMetaEnum::fromType<LanguageSettings::AvailableLanguageEnum>();
    for (int i = 0; i < metaEnum.keyCount(); i++) {
        m_availableLanguages.push_back(LanguageModelData { getLocalLanguageName(static_cast<LanguageSettings::AvailableLanguageEnum>(i)),
                                                           static_cast<LanguageSettings::AvailableLanguageEnum>(i) });
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

QString LanguageModel::getLocalLanguageName(const LanguageSettings::AvailableLanguageEnum language)
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

