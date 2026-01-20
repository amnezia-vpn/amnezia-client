#ifndef PROTOCOLSUICONTROLLER_H
#define PROTOCOLSUICONTROLLER_H

#include <QObject>
#include <QJsonObject>

#include "ui/models/protocolsModel.h"

class ProtocolsUiController : public QObject
{
    Q_OBJECT
    
public:
    explicit ProtocolsUiController(ProtocolsModel* protocolsModel, QObject *parent = nullptr);
    
public slots:
    void updateProtocols(const QJsonObject &config);
    QJsonObject getProtocolsConfig();
    
    void openServerSettings(int protocolIndex);
    void openClientSettings(int protocolIndex);
    
    int defaultPort(int protocolIndex);
    int getPortForInstall(int protocolIndex);
    int defaultTransportProto(int protocolIndex);
    bool defaultPortChangeable(int protocolIndex);
    bool defaultTransportProtoChangeable(int protocolIndex);

signals:
    void openWireGuardServerSettings();
    void openAwgServerSettings();
    void openOpenVpnServerSettings();
    void openXrayServerSettings();
    void openSftpServerSettings();
    void openIkev2ServerSettings();
    void openSocks5ProxyServerSettings();
    
    void openWireGuardClientSettings();
    void openAwgClientSettings();

private:
    ProtocolsModel* m_protocolsModel;
};

#endif // PROTOCOLSUICONTROLLER_H

