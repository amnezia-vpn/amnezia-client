#include "languageModel.h"

LanguageModel::LanguageModel(std::shared_ptr<Settings> settings, QObject *parent)
    : m_settings(settings), QAbstractListModel(parent)
{
    QMetaEnum metaEnum = QMetaEnum::fromType<LanguageSettings::AvailableLanguageEnum>();
    for (int i = 0; i < metaEnum.keyCount(); i++) {
        m_availableLanguages.push_back(
                LanguageModelData { metaEnum.valueToKey(i), static_cast<LanguageSettings::AvailableLanguageEnum>(i) });
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

void LanguageModel::changeLanguage(const LanguageSettings::AvailableLanguageEnum language)
{
    switch (language) {
    case LanguageSettings::AvailableLanguageEnum::English: emit updateTranslations(QLocale::English); break;
    case LanguageSettings::AvailableLanguageEnum::Russian: emit updateTranslations(QLocale::Russian); break;
    default: emit updateTranslations(QLocale::English); break;
    }
}

int LanguageModel::getCurrentLanguageIndex()
{
    auto locale = m_settings->getAppLanguage();
    switch (locale.language()) {
    case QLocale::English: return static_cast<int>(LanguageSettings::AvailableLanguageEnum::English); break;
    case QLocale::Russian: return static_cast<int>(LanguageSettings::AvailableLanguageEnum::Russian); break;
    default: return static_cast<int>(LanguageSettings::AvailableLanguageEnum::English); break;
    }
}

QString LanguageModel::getCurrentLanuageName()
{
    return m_availableLanguages[getCurrentLanguageIndex()].name;
}
