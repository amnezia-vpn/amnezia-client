#ifndef CONFIGCONTROLLER_H
#define CONFIGCONTROLLER_H

#include <QObject>
#include <QSharedPointer>

#include "core/defs.h"
#include "core/models/servers/serverConfig.h"
#include "core/models/containers/containers_defs.h"

class Settings;

using namespace amnezia;

class ConfigController : public QObject
{
    Q_OBJECT

public:
    explicit ConfigController(std::shared_ptr<Settings> settings, QObject *parent = nullptr);
    virtual ~ConfigController() = default;

    // Basic server management
    virtual void addServer(const QSharedPointer<ServerConfig> &serverConfig);
    virtual void editServer(const QSharedPointer<ServerConfig> &serverConfig, int serverIndex);
    virtual void removeServer(int serverIndex);
    
    // Default settings management
    void setDefaultServer(int serverIndex);
    void setDefaultContainer(int serverIndex, int containerIndex);
    
    // API server utilities
    bool isServerFromApiAlreadyExists(quint16 crc) const;
    bool isServerFromApiAlreadyExists(const QString &userCountryCode, 
                                     const QString &serviceType, 
                                     const QString &serviceProtocol) const;
    bool isApiKeyExpired(int serverIndex) const;
    void removeApiConfig(int serverIndex);
    
    // General utilities
    QStringList getAllInstalledServicesName(int serverIndex) const;
    void clearCachedProfile(int serverIndex, DockerContainer container);

    // Split tunneling detection
    bool isDefaultServerDefaultContainerHasSplitTunneling() const;

protected:
    std::shared_ptr<Settings> m_settings;
    
    // Protected helper methods for derived classes
    void updateServerInSettings(const QSharedPointer<ServerConfig> &serverConfig, int serverIndex);

    bool checkSplitTunnelingInContainer(const ContainerConfig &containerConfig, const QString &defaultContainer) const;

signals:
    // Common server management signals
    void serverAdded(int serverIndex);
    void serverEdited(int serverIndex);
    void serverRemoved(int serverIndex);
    void defaultServerChanged(int serverIndex);
    void defaultContainerChanged(int serverIndex, DockerContainer container);
};

#endif // CONFIGCONTROLLER_H 
