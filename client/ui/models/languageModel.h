#ifndef LANGUAGEMODEL_H
#define LANGUAGEMODEL_H

#include <QAbstractListModel>
#include <QQmlEngine>

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
        Hindi
    };
    Q_ENUM_NS(AvailableLanguageEnum)

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
    QString getLocalLanguageName(const LanguageSettings::AvailableLanguageEnum language);

    QVector<LanguageModelData> m_availableLanguages;
};

#endif // LANGUAGEMODEL_H
