#include "xrayConfigSnapshotsModel.h"

#include <QJsonDocument>
#include <QUuid>

#include "core/repositories/secureAppSettingsRepository.h"
#include "core/utils/constants/configKeys.h"

QJsonObject XrayConfigSnapshot::toJson() const
{
    QJsonObject obj;
    obj["id"] = id;
    obj["displayName"] = displayName;
    obj["createdAt"] = createdAt.toString(Qt::ISODate);
    obj["serverConfig"] = serverConfig.toJson();
    return obj;
}

XrayConfigSnapshot XrayConfigSnapshot::fromJson(const QJsonObject &json)
{
    XrayConfigSnapshot s;
    s.id = json.value("id").toString();
    s.displayName = json.value("displayName").toString();
    s.createdAt = QDateTime::fromString(json.value("createdAt").toString(), Qt::ISODate);
    s.serverConfig = amnezia::XrayServerConfig::fromJson(json.value("serverConfig").toObject());
    return s;
}

XrayConfigSnapshotsModel::XrayConfigSnapshotsModel(SecureAppSettingsRepository *appSettings,
                                                   XrayConfigModel *xrayConfigModel, QObject *parent)
    : QAbstractListModel(parent), m_appSettings(appSettings), m_xrayConfigModel(xrayConfigModel)
{
    loadAll();
}

void XrayConfigSnapshotsModel::loadAll()
{
    m_configs.clear();
    QByteArray raw = m_appSettings->xraySavedConfigs();
    if (raw.isEmpty()) {
        return;
    }

    QJsonArray arr = QJsonDocument::fromJson(raw).array();
    for (const QJsonValue &v : arr) {
        m_configs.append(XrayConfigSnapshot::fromJson(v.toObject()));
    }
}

void XrayConfigSnapshotsModel::persistAll()
{
    QJsonArray arr;
    for (const XrayConfigSnapshot &s : m_configs) {
        arr.append(s.toJson());
    }
    m_appSettings->setXraySavedConfigs(QJsonDocument(arr).toJson(QJsonDocument::Compact));
}

int XrayConfigSnapshotsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_configs.size();
}

QVariant XrayConfigSnapshotsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_configs.size()) {
        return QVariant();
    }

    const XrayConfigSnapshot &s = m_configs.at(index.row());

    switch (role) {
    case IdRole: {
        return s.id;
    }
    case DisplayNameRole: {
        return s.displayName;
    }
    case CreatedAtRole: {
        return s.createdAt.toString("dd.MM.yyyy HH:mm");
    }
    }
    return QVariant();
}

QHash<int, QByteArray> XrayConfigSnapshotsModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[IdRole] = "configId";
    roles[DisplayNameRole] = "configName";
    roles[CreatedAtRole] = "configDate";
    return roles;
}

void XrayConfigSnapshotsModel::reload()
{
    beginResetModel();
    loadAll();
    endResetModel();
}

void XrayConfigSnapshotsModel::createFromCurrent(const amnezia::XrayServerConfig &serverConfig)
{
    XrayConfigSnapshot snapshot;
    snapshot.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    snapshot.displayName = buildDisplayName(serverConfig);
    snapshot.createdAt = QDateTime::currentDateTime();
    snapshot.serverConfig = serverConfig;

    beginInsertRows(QModelIndex(), m_configs.size(), m_configs.size());
    m_configs.append(snapshot);
    endInsertRows();

    persistAll();
}

amnezia::XrayServerConfig XrayConfigSnapshotsModel::applyConfig(int index) const
{
    if (index < 0 || index >= m_configs.size()) {
        return amnezia::XrayServerConfig {};
    }

    return m_configs.at(index).serverConfig;
}

void XrayConfigSnapshotsModel::removeConfig(int index)
{
    if (index < 0 || index >= m_configs.size()) {
        return;
    }

    beginRemoveRows(QModelIndex(), index, index);
    m_configs.removeAt(index);
    endRemoveRows();

    persistAll();
    emit configRemoved(index);
}

QString XrayConfigSnapshotsModel::exportToJson(int index) const
{
    if (index < 0 || index >= m_configs.size()) {
        return {};
    }
    return QString::fromUtf8(QJsonDocument(m_configs.at(index).toJson()).toJson(QJsonDocument::Indented));
}

bool XrayConfigSnapshotsModel::importFromJson(const QString &jsonString)
{
    QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8());
    if (!doc.isObject()) {
        emit importFailed(tr("Invalid JSON format"));
        return false;
    }

    XrayConfigSnapshot snapshot = XrayConfigSnapshot::fromJson(doc.object());
    if (snapshot.id.isEmpty()) {
        snapshot.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    if (snapshot.displayName.isEmpty()) {
        snapshot.displayName = buildDisplayName(snapshot.serverConfig);
    }
    snapshot.createdAt = QDateTime::currentDateTime();

    beginInsertRows(QModelIndex(), m_configs.size(), m_configs.size());
    m_configs.append(snapshot);
    endInsertRows();

    persistAll();
    return true;
}

QString XrayConfigSnapshotsModel::buildDisplayName(const amnezia::XrayServerConfig &cfg)
{
    // Build a human-readable name: "XHTTP TLS Reality", "RAW Reality", etc.
    QString transport;
    if (cfg.transport == "xhttp") {
        transport = "XHTTP";
    } else if (cfg.transport == "mkcp") {
        transport = "mKCP";
    } else {
        transport = "RAW (TCP)";
    }

    QString security;
    if (cfg.security == "tls") {
        security = "TLS";
    } else if (cfg.security == "reality") {
        security = "Reality";
    } else {
        security = "None";
    }

    return QString("%1 %2").arg(transport, security).trimmed();
}

void XrayConfigSnapshotsModel::createFromCurrentModel()
{
    if (!m_xrayConfigModel) {
        return;
    }
    createFromCurrent(m_xrayConfigModel->getProtocolConfig().serverConfig);
}

void XrayConfigSnapshotsModel::applyConfigToCurrentModel(int index)
{
    if (!m_xrayConfigModel) {
        return;
    }
    amnezia::XrayServerConfig cfg = applyConfig(index);
    if (cfg.port.isEmpty()) {
        return; // guard against invalid index
    }
    m_xrayConfigModel->applyServerConfig(cfg);
}
