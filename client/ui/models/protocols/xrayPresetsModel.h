#ifndef XRAYPRESETSMODEL_H
#define XRAYPRESETSMODEL_H

#include <QAbstractListModel>
#include <optional>

#include "core/models/protocols/xrayPresets.h"

class SecureAppSettingsRepository;

class XrayPresetsModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int customCount READ customCount NOTIFY presetsChanged)

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        DescriptionRole,
        CreatedAtRole,
        BuiltinRole,
        RecommendedRole,
        SelectedRole,
    };

    explicit XrayPresetsModel(SecureAppSettingsRepository *appSettings, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int customCount() const;

    std::optional<amnezia::XrayPreset> preset(const QString &id) const;
    void setServer(const QString &site, const QString &port);
    void setSelectedId(const QString &id);

    Q_INVOKABLE QString presetName(const QString &id) const;
    Q_INVOKABLE QString presetJson(const QString &id) const;
    Q_INVOKABLE QString addPreset(const QString &name, const QString &inbound);
    Q_INVOKABLE void removePreset(const QString &id);

signals:
    void presetsChanged();

private:
    void rebuild();
    void saveCustom();

    SecureAppSettingsRepository *m_appSettings;
    QString m_site;
    QString m_port;
    QString m_selectedId;
    QList<amnezia::XrayPreset> m_custom;
    QList<amnezia::XrayPreset> m_presets;
};

#endif // XRAYPRESETSMODEL_H
