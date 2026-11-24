#include "xrayPresetsModel.h"

#include <algorithm>

#include "logger.h"

#include "core/repositories/secureAppSettingsRepository.h"

using namespace amnezia;

namespace
{
    Logger logger("XrayPresetsModel");
}

XrayPresetsModel::XrayPresetsModel(SecureAppSettingsRepository *appSettings, QObject *parent)
    : QAbstractListModel(parent), m_appSettings(appSettings)
{
    const QByteArray stored = m_appSettings->xrayPresets();
    if (stored.isEmpty()) {
        const QByteArray legacy = m_appSettings->takeXrayLegacySnapshots();
        if (!legacy.isEmpty()) {
            m_custom = xrayPresets::customFromLegacySnapshots(legacy);
            logger.info() << "migrated legacy XRay snapshots into custom presets:" << QString::number(m_custom.size());
            saveCustom();
        }
    } else {
        m_custom = xrayPresets::customFromStorage(stored);
        if (xrayPresets::ensureUniqueIds(m_custom)) {
            saveCustom();
        }
    }
    logger.info() << "created, custom presets" << QString::number(m_custom.size());
    rebuild();
}

int XrayPresetsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_presets.size();
}

QVariant XrayPresetsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_presets.size()) {
        return {};
    }
    const XrayPreset &item = m_presets.at(index.row());
    switch (role) {
    case IdRole: return item.id;
    case NameRole: return item.name;
    case DescriptionRole: return item.description;
    case CreatedAtRole:
        return item.createdAt.isValid() ? item.createdAt.toString(QStringLiteral("dd.MM.yyyy HH:mm")) : QString();
    case BuiltinRole: return item.builtin;
    case RecommendedRole: return item.recommended;
    case SelectedRole: return item.id == m_selectedId;
    default: return {};
    }
}

QHash<int, QByteArray> XrayPresetsModel::roleNames() const
{
    return {
        { IdRole, "presetId" },         { NameRole, "name" },       { DescriptionRole, "description" },
        { CreatedAtRole, "createdAt" }, { BuiltinRole, "builtin" }, { RecommendedRole, "recommended" },
        { SelectedRole, "selected" },
    };
}

int XrayPresetsModel::customCount() const
{
    return m_custom.size();
}

std::optional<XrayPreset> XrayPresetsModel::preset(const QString &id) const
{
    for (const XrayPreset &item : m_presets) {
        if (item.id == id) {
            return item;
        }
    }
    return std::nullopt;
}

void XrayPresetsModel::setServer(const QString &site, const QString &port)
{
    if (m_site == site && m_port == port) {
        return;
    }
    logger.info() << "setServer: site" << site << "port" << port;
    m_site = site;
    m_port = port;
    rebuild();
}

void XrayPresetsModel::setSelectedId(const QString &id)
{
    if (m_selectedId == id) {
        return;
    }
    logger.info() << "setSelectedId:" << m_selectedId << "->" << id;
    m_selectedId = id;
    if (!m_presets.isEmpty()) {
        emit dataChanged(index(0), index(m_presets.size() - 1), { SelectedRole });
    }
}

QString XrayPresetsModel::presetName(const QString &id) const
{
    const auto item = preset(id);
    return item ? item->name : QString();
}

QString XrayPresetsModel::presetJson(const QString &id) const
{
    const auto item = preset(id);
    logger.info() << "presetJson:" << id << (item ? "found" : "missing");
    return item ? item->inbound : QString();
}

QString XrayPresetsModel::addPreset(const QString &name, const QString &inbound)
{
    const XrayPreset item = xrayPresets::makeCustom(name, inbound);
    m_custom.append(item);
    saveCustom();
    rebuild();
    logger.info() << "addPreset:" << item.id << item.name << "custom now" << QString::number(m_custom.size());
    return item.id;
}

void XrayPresetsModel::removePreset(const QString &id)
{
    const auto before = m_custom.size();
    m_custom.erase(
            std::remove_if(m_custom.begin(), m_custom.end(), [&id](const XrayPreset &item) { return item.id == id; }),
            m_custom.end());
    logger.info() << "removePreset:" << id << "removed" << QString::number(before - m_custom.size());
    saveCustom();
    rebuild();
}

void XrayPresetsModel::rebuild()
{
    beginResetModel();
    m_presets = m_site.isEmpty() ? QList<XrayPreset>() : xrayPresets::builtins(m_site, m_port);
    m_presets.append(m_custom);
    endResetModel();
    logger.info() << "rebuild: total" << QString::number(m_presets.size()) << "custom"
                  << QString::number(m_custom.size()) << "selected" << m_selectedId;
    emit presetsChanged();
}

void XrayPresetsModel::saveCustom()
{
    m_appSettings->setXrayPresets(xrayPresets::customToStorage(m_custom));
}
