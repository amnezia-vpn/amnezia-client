#ifndef XRAYCONFIGSMODEL_H
#define XRAYCONFIGSMODEL_H

#include <QAbstractListModel>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QVector>

#include "core/models/protocols/xrayProtocolConfig.h"

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

class XrayConfigsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        DisplayNameRole,
        CreatedAtRole, // "dd.MM.yyyy HH:mm"
    };

    explicit XrayConfigsModel(SecureAppSettingsRepository *appSettings, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

public slots:
    void reload();

    Q_INVOKABLE void createFromCurrent(const amnezia::XrayServerConfig &serverConfig);
    Q_INVOKABLE amnezia::XrayServerConfig applyConfig(int index) const;
    Q_INVOKABLE void removeConfig(int index);

    Q_INVOKABLE QString exportToJson(int index) const;
    Q_INVOKABLE bool importFromJson(const QString &jsonString);

signals:
    void configApplied(int index);
    void configRemoved(int index);
    void importFailed(const QString &errorMessage);

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    SecureAppSettingsRepository *m_appSettings;
    QVector<XrayConfigSnapshot> m_configs;

    void persistAll();
    void loadAll();
    static QString buildDisplayName(const amnezia::XrayServerConfig &cfg);
};

#endif // XRAYCONFIGSMODEL_H
