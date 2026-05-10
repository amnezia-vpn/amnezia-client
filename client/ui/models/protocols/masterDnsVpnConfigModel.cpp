#include "masterDnsVpnConfigModel.h"

#include <QJsonArray>
#include <QRegularExpression>

#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/utils/protocolEnum.h"

using namespace amnezia;
using namespace ProtocolUtils;

MasterDnsVpnConfigModel::MasterDnsVpnConfigModel(QObject *parent) : QAbstractListModel(parent) {}

int MasterDnsVpnConfigModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

bool MasterDnsVpnConfigModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return false;
    }

    switch (role) {
    case Roles::DomainsRole: {
        // QML hands us either a JSON array (string-form), an array literal, or
        // a comma-separated string. Normalise to a QJsonArray.
        QJsonArray arr;
        if (value.canConvert<QStringList>()) {
            for (const QString &s : value.toStringList()) {
                const QString trimmed = s.trimmed();
                if (!trimmed.isEmpty()) {
                    arr.append(trimmed);
                }
            }
        } else {
            const QString raw = value.toString();
            for (const QString &part : raw.split(QRegularExpression(QStringLiteral("[,\\n]")))) {
                const QString trimmed = part.trimmed();
                if (!trimmed.isEmpty()) {
                    arr.append(trimmed);
                }
            }
        }
        m_protocolConfig.serverConfig.domains = arr;
        break;
    }
    case Roles::PortRole:
        m_protocolConfig.serverConfig.port = value.toString();
        break;
    case Roles::EncryptionMethodRole:
        m_protocolConfig.serverConfig.encryptionMethod = value.toInt();
        break;
    case Roles::ProtocolTypeRole:
        m_protocolConfig.serverConfig.protocolType = value.toString();
        break;
    case Roles::ListenPortRole:
        if (m_protocolConfig.clientConfig.has_value()) {
            m_protocolConfig.clientConfig->listenPort = value.toString();
        }
        break;
    default:
        return false;
    }

    emit dataChanged(index, index, QList { role });
    return true;
}

QVariant MasterDnsVpnConfigModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return QVariant();
    }

    switch (role) {
    case Roles::DomainsRole: {
        QStringList out;
        for (const QJsonValue &v : m_protocolConfig.serverConfig.domains) {
            if (v.isString()) {
                out.append(v.toString());
            }
        }
        return out;
    }
    case Roles::PortRole:
        return m_protocolConfig.serverConfig.port;
    case Roles::EncryptionMethodRole:
        return m_protocolConfig.serverConfig.encryptionMethod;
    case Roles::ProtocolTypeRole:
        return m_protocolConfig.serverConfig.protocolType;
    case Roles::ListenPortRole:
        if (m_protocolConfig.clientConfig.has_value()) {
            return m_protocolConfig.clientConfig->listenPort;
        }
        return QString::fromLatin1(protocols::masterDnsVpn::defaultLocalProxyPort);
    }

    return QVariant();
}

void MasterDnsVpnConfigModel::updateModel(amnezia::DockerContainer container,
                                          const amnezia::MasterDnsVpnProtocolConfig &protocolConfig)
{
    beginResetModel();
    m_container = container;
    m_protocolConfig = protocolConfig;
    applyDefaultsToServerConfig(m_protocolConfig.serverConfig);
    m_originalProtocolConfig = m_protocolConfig;
    endResetModel();
}

void MasterDnsVpnConfigModel::applyDefaultsToServerConfig(
        amnezia::MasterDnsVpnServerConfig &config)
{
    if (config.port.isEmpty()) {
        config.port = QString::fromLatin1(protocols::masterDnsVpn::defaultPort);
    }
    if (config.bind.isEmpty()) {
        config.bind = QStringLiteral("0.0.0.0");
    }
    if (config.encryptionMethod < protocols::masterDnsVpn::encryptionMethodNone
        || config.encryptionMethod > protocols::masterDnsVpn::encryptionMethodAes256Gcm) {
        config.encryptionMethod = protocols::masterDnsVpn::defaultEncryptionMethod;
    }
    if (config.protocolType.isEmpty()) {
        config.protocolType = QStringLiteral("SOCKS5");
    }
}

amnezia::MasterDnsVpnProtocolConfig MasterDnsVpnConfigModel::getProtocolConfig()
{
    // If the operator changed the encryption key or the domain set, every
    // existing per-peer client_config.toml is now invalid (different key
    // = different tunnel; different domain = different NS delegation).
    // Drop the stale client config slot so the UI re-prompts.
    const bool keyChanged = m_protocolConfig.serverConfig.encryptionKey
            != m_originalProtocolConfig.serverConfig.encryptionKey;
    const bool domainsChanged = m_protocolConfig.serverConfig.domains
            != m_originalProtocolConfig.serverConfig.domains;
    if (keyChanged || domainsChanged) {
        m_protocolConfig.clearClientConfig();
    }

    return m_protocolConfig;
}

QHash<int, QByteArray> MasterDnsVpnConfigModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[DomainsRole] = "domains";
    roles[PortRole] = "port";
    roles[EncryptionMethodRole] = "encryptionMethod";
    roles[ProtocolTypeRole] = "protocolType";
    roles[ListenPortRole] = "listenPort";
    return roles;
}
