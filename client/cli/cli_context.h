#ifndef CLI_CONTEXT_H
#define CLI_CONTEXT_H

#include <QJsonObject>
#include <QObject>
#include <QScopedPointer>
#include <QSharedPointer>
#include <QThread>

#include "cli_common.h"
#include "containers/containers_defs.h"
#include "core/defs.h"
#include "protocols/vpnprotocol.h"
#include "settings.h"

class ApiConfigsController;
class ApiAccountInfoModel;
class ApiBenefitsModel;
class ApiCountryModel;
class ApiDevicesModel;
class ApiSettingsController;
class ApiServicesModel;
class ApiSubscriptionPlansModel;
class ClientManagementModel;
class ConnectionController;
class ContainersModel;
class ImportController;
class InstallController;
class ProtocolsModel;
class ServersModel;
class VpnConnection;

namespace cli
{

QString connectionStateKey(Vpn::ConnectionState state);
QString displayContainerName(amnezia::DockerContainer container);
amnezia::DockerContainer containerFromCliName(const QString &name);
QStringList availableContainerNames();

class Context : public QObject
{
    Q_OBJECT

public:
    explicit Context(QObject *parent = nullptr);
    ~Context() override;

    Result reload();

    Result listServers();
    Result showServer(int requestedIndex);
    Result addServer(const QString &name, const amnezia::ServerCredentials &credentials);
    Result importConfigFromFile(const QString &fileName);
    Result importConfigFromData(const QString &data);
    Result removeServer(int requestedIndex);
    Result setDefaultServer(int requestedIndex);
    Result scanServer(int requestedIndex);
    Result listCountries(int requestedIndex);
    Result setCountry(int requestedIndex, const QString &countryCode);

    Result listContainers(int requestedIndex);
    Result setDefaultContainer(int requestedIndex, amnezia::DockerContainer container);
    Result removeContainer(int requestedIndex, amnezia::DockerContainer container);

    Result installServer(const QString &name, const amnezia::ServerCredentials &credentials, amnezia::DockerContainer container,
                         int containerPort, amnezia::TransportProto transport, const QString &keyPassphrase = {});
    Result installContainer(int requestedIndex, amnezia::DockerContainer container, int containerPort, amnezia::TransportProto transport,
                            const QString &keyPassphrase = {});

    Result startConnection(int requestedIndex);
    Result stopConnection();
    Result cleanupLogs();

    QJsonObject status() const;

signals:
    void statusChanged();

private:
    void registerMetaTypes();
    bool hasServers() const;
    Result resolveServerIndexResult(int requestedIndex, int &resolvedIndex) const;
    void activateServer(int resolvedIndex, bool alsoSetDefault);
    void refreshActiveContainer();

    void setLastError(amnezia::ErrorCode error);
    void clearLastError();

    QJsonObject serverSummary(int index) const;
    QJsonObject containerSummary(const QJsonObject &containerConfig, const QString &defaultContainerName) const;
    QJsonArray containerSummaries(const QJsonObject &serverConfig) const;

    std::shared_ptr<Settings> m_settings;
    QSharedPointer<ServersModel> m_serversModel;
    QSharedPointer<ContainersModel> m_containersModel;
    QSharedPointer<ProtocolsModel> m_protocolsModel;
    QSharedPointer<ClientManagementModel> m_clientManagementModel;
    QSharedPointer<ApiAccountInfoModel> m_apiAccountInfoModel;
    QSharedPointer<ApiCountryModel> m_apiCountryModel;
    QSharedPointer<ApiDevicesModel> m_apiDevicesModel;
    QSharedPointer<ApiServicesModel> m_apiServicesModel;
    QSharedPointer<ApiSubscriptionPlansModel> m_apiSubscriptionPlansModel;
    QSharedPointer<ApiBenefitsModel> m_apiBenefitsModel;

    QScopedPointer<ConnectionController> m_connectionController;
    QScopedPointer<InstallController> m_installController;
    QScopedPointer<ImportController> m_importController;
    QScopedPointer<ApiConfigsController> m_apiConfigsController;
    QScopedPointer<ApiSettingsController> m_apiSettingsController;

    QSharedPointer<VpnConnection> m_vpnConnection;
    QThread m_vpnConnectionThread;

    Vpn::ConnectionState m_state = Vpn::ConnectionState::Disconnected;
    amnezia::ErrorCode m_lastError = amnezia::ErrorCode::NoError;
    int m_activeServerIndex = -1;
    amnezia::DockerContainer m_activeContainer = amnezia::DockerContainer::None;
    quint64 m_totalReceivedBytes = 0;
    quint64 m_totalSentBytes = 0;
};

} // namespace cli

#endif // CLI_CONTEXT_H
