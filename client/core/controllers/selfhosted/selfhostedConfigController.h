#ifndef SELFHOSTEDCONFIGCONTROLLER_H
#define SELFHOSTEDCONFIGCONTROLLER_H

#include "../configController.h"
#include "core/models/containers/containerConfig.h"
#include "core/models/protocols/protocolConfig.h"

#include <QMap>

using namespace amnezia;

class SelfhostedConfigController : public ConfigController
{
    Q_OBJECT

public:
    explicit SelfhostedConfigController(std::shared_ptr<Settings> settings, QObject *parent = nullptr);

    // Self-hosted specific functionality
    
    // Amnezia DNS management
    void toggleAmneziaDns(bool enabled);
    QPair<QString, QString> getDnsPair(int serverIndex) const;
    bool isAmneziaDnsContainerInstalled(int serverIndex) const;
    
    // Protocol management (from ProtocolConfigController)
    QMap<QString, QSharedPointer<ProtocolConfig>> getProtocolConfigs(const QVector<QSharedPointer<ProtocolConfig>> &protocols);
    void updateProtocolConfiguration(Proto protocol, const QSharedPointer<ProtocolConfig> &protocolConfig);
    QSharedPointer<ProtocolConfig> createProtocolConfig(Proto protocol);
    bool isProtocolSupported(Proto protocol) const;

private:
    bool m_isAmneziaDnsEnabled;
    
    // Helper methods
    bool checkSplitTunnelingInContainer(const ContainerConfig &containerConfig, 
                                       const QString &defaultContainer) const;

signals:
    // Self-hosted specific signals
    void amneziaDnsToggled(bool enabled);
    void protocolConfigUpdated(Proto protocol, const QSharedPointer<ProtocolConfig> &config);
};

#endif // SELFHOSTEDCONFIGCONTROLLER_H 
