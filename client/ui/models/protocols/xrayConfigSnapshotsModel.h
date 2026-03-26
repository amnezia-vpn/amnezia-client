#ifndef XRAYCONFIGSMODEL_H
#define XRAYCONFIGSMODEL_H

#include <QAbstractListModel>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QVector>

#include "core/models/protocols/xrayProtocolConfig.h"
#include "ui/models/protocols/xrayConfigModel.h"

class SecureAppSettingsRepository;

struct XrayConfigSnapshot
{
    QString id;
    QString displayName; // auto-generated: "XHTTP TLS Reality", "RAW Reality", etc.
    QDateTime createdAt;
    amnezia::XrayServerConfig serverConfig;

    QJsonObject toJson() const;
    static XrayConfigSnapshot fromJson(const QJsonObject &json);
};

class XrayConfigSnapshotsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        DisplayNameRole,
        CreatedAtRole, // "dd.MM.yyyy HH:mm"
    };

    explicit XrayConfigSnapshotsModel(SecureAppSettingsRepository *appSettings, XrayConfigModel *xrayConfigModel,
                                      QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

public slots:
    void reload();

    Q_INVOKABLE void createFromCurrent(const amnezia::XrayServerConfig &serverConfig);
    Q_INVOKABLE amnezia::XrayServerConfig applyConfig(int index) const;
    Q_INVOKABLE void removeConfig(int index);

    Q_INVOKABLE QString exportToJson(int index) const;
    Q_INVOKABLE bool importFromJson(const QString &jsonString);

    // Convenience: create snapshot from live model, apply snapshot back to model
    Q_INVOKABLE void createFromCurrentModel();
    Q_INVOKABLE void applyConfigToCurrentModel(int index);

signals:
    void configApplied(int index);
    void configRemoved(int index);
    void importFailed(const QString &errorMessage);

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    SecureAppSettingsRepository *m_appSettings;
    XrayConfigModel *m_xrayConfigModel;
    QVector<XrayConfigSnapshot> m_configs;

    void persistAll();
    void loadAll();
    static QString buildDisplayName(const amnezia::XrayServerConfig &cfg);
};

#endif // XRAYCONFIGSMODEL_H
