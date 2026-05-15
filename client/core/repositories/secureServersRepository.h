#ifndef SECURESERVERSREPOSITORY_H
#define SECURESERVERSREPOSITORY_H

#include <QHash>
#include <QJsonObject>
#include <QObject>
#include <QVector>
#include <QtGlobal>
#include <optional>

#include "core/models/selfhosted/selfHostedAdminServerConfig.h"
#include "core/models/selfhosted/selfHostedUserServerConfig.h"
#include "core/models/selfhosted/nativeServerConfig.h"
#include "core/models/api/apiV2ServerConfig.h"
#include "core/models/api/legacyApiServerConfig.h"
#include "core/models/containerConfig.h"
#include "core/utils/routeModes.h"
#include "core/utils/serverConfigUtils.h"
#include "secureQSettings.h"

using namespace amnezia;

class SecureServersRepository : public QObject
{
    Q_OBJECT

public:
    explicit SecureServersRepository(SecureQSettings *settings, QObject *parent = nullptr);

    QString addServer(const QString &serverId, const QJsonObject &serverJson, serverConfigUtils::ConfigType kind);
    void editServer(const QString &serverId, const QJsonObject &serverJson, serverConfigUtils::ConfigType kind);
    void removeServer(const QString &serverId);
    serverConfigUtils::ConfigType serverKind(const QString &serverId) const;

    std::optional<SelfHostedAdminServerConfig> selfHostedAdminConfig(const QString &serverId) const;
    std::optional<SelfHostedUserServerConfig> selfHostedUserConfig(const QString &serverId) const;
    std::optional<NativeServerConfig> nativeConfig(const QString &serverId) const;
    std::optional<ApiV2ServerConfig> apiV2Config(const QString &serverId) const;
    std::optional<LegacyApiServerConfig> legacyApiConfig(const QString &serverId) const;

    int serversCount() const;
    int indexOfServerId(const QString &serverId) const;
    QString serverIdAt(int index) const;
    QVector<QString> orderedServerIds() const;

    int defaultServerIndex() const;
    QString defaultServerId() const;
    void setDefaultServer(const QString &serverId);

    QJsonObject serverJson(int index) const;
    void editServerJson(int index, const QJsonObject &serverJson);

    QVariantMap managedVpnSites(int serverIndex, RouteMode mode) const;
    QVariantMap managedVpnSitesForRouting(int serverIndex, RouteMode mode) const;
    void setManagedVpnSites(int serverIndex, RouteMode mode, const QVariantMap &sites);
    bool addManagedVpnSite(int serverIndex, RouteMode mode, const QString &site, const QString &ip = "");
    void addManagedVpnSites(int serverIndex, RouteMode mode, const QMap<QString, QString> &sites);
    void removeManagedVpnSite(int serverIndex, RouteMode mode, const QString &site);
    void removeAllManagedVpnSites(int serverIndex, RouteMode mode);
    bool isManagedSplitTunnelingForceEnabled(int serverIndex) const;
    void setManagedSplitTunnelingForceEnabled(int serverIndex, bool enabled);
    RouteMode effectiveSiteRouteMode(int serverIndex, bool localSplitEnabled, RouteMode localRouteMode) const;

    ServerCredentials serverCredentials(int index) const;
    bool hasServerWithVpnKey(const QString &vpnKey) const;
    bool hasServerWithCrc(quint16 crc) const;
    void clearServers();

    void invalidateCache();

signals:
    void serverAdded(const QString &serverId);
    void serverEdited(const QString &serverId);
    void serverRemoved(const QString &serverId, int removedIndex);
    void defaultServerChanged(const QString &defaultServerId);

private:
    void loadFromStorage();
    void updateDefaultServerFromStorage();
    void persistDefaultServerFields();

    QString normalizedOrGeneratedServerId(const QString &candidateId) const;

    void syncToStorage();
    QVariant value(const QString &key, const QVariant &defaultValue) const;
    void setValue(const QString &key, const QVariant &value);

    void clearServerStateMaps();

    SecureQSettings *m_settings;

    QHash<QString, QJsonObject> m_serverJsonById;
    QVector<QString> m_orderedServerIds;

    QString m_defaultServerId;
};

#endif // SECURESERVERSREPOSITORY_H
