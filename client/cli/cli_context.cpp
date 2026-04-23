#include "cli_context.h"

#include <QCoreApplication>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaEnum>
#include <QSysInfo>

#include "core/api/apiDefs.h"
#include "logger.h"
#include "core/errorstrings.h"
#include "ui/controllers/api/apiSettingsController.h"
#include "ui/controllers/api/apiConfigsController.h"
#include "ui/controllers/connectionController.h"
#include "ui/controllers/importController.h"
#include "ui/controllers/installController.h"
#include "ui/models/api/apiAccountInfoModel.h"
#include "ui/models/api/apiBenefitsModel.h"
#include "ui/models/api/apiCountryModel.h"
#include "ui/models/api/apiDevicesModel.h"
#include "ui/models/api/apiServicesModel.h"
#include "ui/models/api/apiSubscriptionPlansModel.h"
#include "ui/models/clientManagementModel.h"
#include "ui/models/containers_model.h"
#include "ui/models/protocols_model.h"
#include "ui/models/servers_model.h"
#include "version.h"
#include "vpnconnection.h"

namespace
{

using namespace amnezia;

QString serviceTypeKey(ServiceType serviceType)
{
    switch (serviceType) {
    case ServiceType::None: return "none";
    case ServiceType::Vpn: return "vpn";
    case ServiceType::Other: return "other";
    }

    return "unknown";
}

QString hostWithOptionalPort(const ServerCredentials &credentials)
{
    if (credentials.port > 0 && credentials.port != 22) {
        return QString("%1:%2").arg(credentials.hostName).arg(credentials.port);
    }

    return credentials.hostName;
}

void disconnectTemporaryConnections(const QList<QMetaObject::Connection> &connections)
{
    for (const auto &connection : connections) {
        QObject::disconnect(connection);
    }
}

QJsonObject apiConfigObject(const QJsonObject &serverConfig)
{
    return serverConfig.value(apiDefs::key::apiConfig).toObject();
}

QJsonArray availableCountriesArray(const QJsonObject &serverConfig)
{
    return apiConfigObject(serverConfig).value(apiDefs::key::availableCountries).toArray();
}

QString selectedCountryCode(const QJsonObject &serverConfig)
{
    return apiConfigObject(serverConfig).value(apiDefs::key::serverCountryCode).toString();
}

QString selectedCountryName(const QJsonObject &serverConfig)
{
    return apiConfigObject(serverConfig).value(apiDefs::key::serverCountryName).toString();
}

QHash<QString, QJsonObject> issuedConfigsByCountry(const QJsonArray &issuedConfigs)
{
    QHash<QString, QJsonObject> issuedByCountry;

    for (const auto &value : issuedConfigs) {
        const auto issuedObject = value.toObject();
        if (issuedObject.value(apiDefs::key::sourceType).toString() != QStringLiteral("country_config")) {
            continue;
        }

        issuedByCountry.insert(issuedObject.value(apiDefs::key::serverCountryCode).toString().toLower(), issuedObject);
    }

    return issuedByCountry;
}

QJsonObject countrySummary(const QJsonObject &countryObject, const QString &selectedCountryCode,
                           const QHash<QString, QJsonObject> &issuedByCountry)
{
    const QString countryCode = countryObject.value(apiDefs::key::serverCountryCode).toString();
    const QString normalizedCode = countryCode.toLower();
    const auto issuedObject = issuedByCountry.value(normalizedCode);

    QJsonObject summary;
    summary["code"] = countryCode;
    summary["name"] = countryObject.value(apiDefs::key::serverCountryName).toString();
    summary["selected"] = normalizedCode == selectedCountryCode.toLower();
    summary["issued"] = !issuedObject.isEmpty();
    summary["worker_expired"] =
            issuedObject.value(apiDefs::key::lastDownloaded).toString() < issuedObject.value(apiDefs::key::workerLastUpdated).toString();

    return summary;
}

} // namespace

namespace cli
{

QString connectionStateKey(Vpn::ConnectionState state)
{
    switch (state) {
    case Vpn::ConnectionState::Unknown: return "unknown";
    case Vpn::ConnectionState::Disconnected: return "disconnected";
    case Vpn::ConnectionState::Preparing: return "preparing";
    case Vpn::ConnectionState::Connecting: return "connecting";
    case Vpn::ConnectionState::Connected: return "connected";
    case Vpn::ConnectionState::Disconnecting: return "disconnecting";
    case Vpn::ConnectionState::Reconnecting: return "reconnecting";
    case Vpn::ConnectionState::Error: return "error";
    }

    return "unknown";
}

QString displayContainerName(amnezia::DockerContainer container)
{
    return ContainerProps::containerHumanNames().value(container, ContainerProps::containerTypeToString(container));
}

amnezia::DockerContainer containerFromCliName(const QString &rawName)
{
    const QString name = rawName.trimmed().toLower();
    if (name.isEmpty()) {
        return amnezia::DockerContainer::None;
    }

    const QHash<QString, amnezia::DockerContainer> aliases = {
        { "none", amnezia::DockerContainer::None },
        { "openvpn", amnezia::DockerContainer::OpenVpn },
        { "amnezia-openvpn", amnezia::DockerContainer::OpenVpn },
        { "cloak", amnezia::DockerContainer::Cloak },
        { "openvpn-cloak", amnezia::DockerContainer::Cloak },
        { "amnezia-openvpn-cloak", amnezia::DockerContainer::Cloak },
        { "shadowsocks", amnezia::DockerContainer::ShadowSocks },
        { "openvpn-ss", amnezia::DockerContainer::ShadowSocks },
        { "amnezia-shadowsocks", amnezia::DockerContainer::ShadowSocks },
        { "wireguard", amnezia::DockerContainer::WireGuard },
        { "amnezia-wireguard", amnezia::DockerContainer::WireGuard },
        { "awg", amnezia::DockerContainer::Awg2 },
        { "awg2", amnezia::DockerContainer::Awg2 },
        { "amneziawg", amnezia::DockerContainer::Awg2 },
        { "amneziawg2", amnezia::DockerContainer::Awg2 },
        { "amnezia-awg", amnezia::DockerContainer::Awg },
        { "amnezia-awg2", amnezia::DockerContainer::Awg2 },
        { "xray", amnezia::DockerContainer::Xray },
        { "amnezia-xray", amnezia::DockerContainer::Xray },
        { "ssxray", amnezia::DockerContainer::SSXray },
        { "amnezia-ssxray", amnezia::DockerContainer::SSXray },
        { "ipsec", amnezia::DockerContainer::Ipsec },
        { "ikev2", amnezia::DockerContainer::Ipsec },
        { "amnezia-ipsec", amnezia::DockerContainer::Ipsec },
        { "dns", amnezia::DockerContainer::Dns },
        { "amneziadns", amnezia::DockerContainer::Dns },
        { "amnezia-dns", amnezia::DockerContainer::Dns },
        { "tor", amnezia::DockerContainer::TorWebSite },
        { "torwebsite", amnezia::DockerContainer::TorWebSite },
        { "amnezia-torwebsite", amnezia::DockerContainer::TorWebSite },
        { "sftp", amnezia::DockerContainer::Sftp },
        { "amnezia-sftp", amnezia::DockerContainer::Sftp },
        { "socks5", amnezia::DockerContainer::Socks5Proxy },
        { "socks5proxy", amnezia::DockerContainer::Socks5Proxy },
        { "amnezia-socks5proxy", amnezia::DockerContainer::Socks5Proxy },
    };

    if (aliases.contains(name)) {
        return aliases.value(name);
    }

    const auto parsed = ContainerProps::containerFromString(name);
    if (parsed != amnezia::DockerContainer::None) {
        return parsed;
    }

    for (const auto container : ContainerProps::allContainers()) {
        if (ContainerProps::containerTypeToString(container) == name) {
            return container;
        }
    }

    return amnezia::DockerContainer::None;
}

QStringList availableContainerNames()
{
    return {
        "openvpn",
        "cloak",
        "shadowsocks",
        "wireguard",
        "awg",
        "awg2",
        "xray",
        "ssxray",
        "ikev2",
        "dns",
        "torwebsite",
        "sftp",
        "socks5proxy",
    };
}

Context::Context(QObject *parent)
    : QObject(parent)
{
    registerMetaTypes();

    m_settings = std::make_shared<Settings>();

    m_serversModel.reset(new ServersModel(m_settings, this));
    m_containersModel.reset(new ContainersModel(this));
    m_protocolsModel.reset(new ProtocolsModel(m_settings, this));
    m_clientManagementModel.reset(new ClientManagementModel(m_settings, this));
    m_apiAccountInfoModel.reset(new ApiAccountInfoModel(this));
    m_apiCountryModel.reset(new ApiCountryModel(this));
    m_apiDevicesModel.reset(new ApiDevicesModel(m_settings, this));
    m_apiServicesModel.reset(new ApiServicesModel(this));
    m_apiSubscriptionPlansModel.reset(new ApiSubscriptionPlansModel(this));
    m_apiBenefitsModel.reset(new ApiBenefitsModel(this));

    connect(m_serversModel.get(), &ServersModel::containersUpdated, m_containersModel.get(), &ContainersModel::updateModel);

    m_vpnConnection.reset(new VpnConnection(m_settings));
    m_vpnConnection->moveToThread(&m_vpnConnectionThread);
    m_vpnConnectionThread.start();

    m_connectionController.reset(
            new ConnectionController(m_serversModel, m_containersModel, m_clientManagementModel, m_vpnConnection, m_settings, this));
    m_installController.reset(new InstallController(m_serversModel, m_containersModel, m_protocolsModel, m_clientManagementModel, m_settings, this));
    m_importController.reset(new ImportController(m_serversModel, m_containersModel, m_settings, this));
    m_apiConfigsController.reset(new ApiConfigsController(
            m_serversModel, m_apiServicesModel, m_apiSubscriptionPlansModel, m_apiBenefitsModel, m_settings, this));
    m_apiSettingsController.reset(
            new ApiSettingsController(m_serversModel, m_apiAccountInfoModel, m_apiCountryModel, m_apiDevicesModel, m_settings, this));

    connect(m_connectionController.get(), &ConnectionController::prepareConfig, this, [this]() {
        clearLastError();
        emit m_vpnConnection->connectionStateChanged(Vpn::ConnectionState::Preparing);

        if (!m_apiConfigsController->isConfigValid()) {
            emit m_vpnConnection->connectionStateChanged(Vpn::ConnectionState::Disconnected);
            return;
        }

        m_installController->validateConfig();
    });

    connect(m_installController.get(), &InstallController::configValidated, this, [this](bool isValid) {
        if (!isValid) {
            emit m_vpnConnection->connectionStateChanged(Vpn::ConnectionState::Disconnected);
            return;
        }

        m_connectionController->openConnection();
    });

    connect(m_connectionController.get(), &ConnectionController::connectionErrorOccurred, this, [this](ErrorCode error) {
        setLastError(error);
        emit m_vpnConnection->connectionStateChanged(Vpn::ConnectionState::Disconnected);
    });

    connect(m_installController.get(), &InstallController::installationErrorOccurred, this, &Context::setLastError);
    connect(m_apiConfigsController.get(), &ApiConfigsController::errorOccurred, this, &Context::setLastError);
    connect(m_apiSettingsController.get(), &ApiSettingsController::errorOccurred, this, &Context::setLastError);

    connect(m_vpnConnection.get(), &VpnConnection::connectionStateChanged, this, [this](Vpn::ConnectionState state) {
        m_state = state;
        if (state == Vpn::ConnectionState::Connected) {
            clearLastError();
        }
        if (state == Vpn::ConnectionState::Disconnected && m_lastError == ErrorCode::NoError) {
            m_totalReceivedBytes = 0;
            m_totalSentBytes = 0;
        }
        emit statusChanged();
    });

    connect(m_vpnConnection.get(), &VpnConnection::bytesChanged, this, [this](quint64 receivedBytes, quint64 sentBytes) {
        m_totalReceivedBytes += receivedBytes;
        m_totalSentBytes += sentBytes;
        emit statusChanged();
    });

    reload();
}

Context::~Context()
{
    if (m_vpnConnection && m_vpnConnectionThread.isRunning()) {
        QMetaObject::invokeMethod(m_vpnConnection.get(), "disconnectSlots", Qt::BlockingQueuedConnection);
        QMetaObject::invokeMethod(m_vpnConnection.get(), "disconnectFromVpn", Qt::BlockingQueuedConnection);
    }

    m_vpnConnectionThread.requestInterruption();
    m_vpnConnectionThread.quit();

    if (!m_vpnConnectionThread.wait(3000)) {
        m_vpnConnectionThread.terminate();
        m_vpnConnectionThread.wait(500);
    }
}

void Context::registerMetaTypes()
{
    qRegisterMetaType<amnezia::ServerCredentials>("ServerCredentials");
    qRegisterMetaType<amnezia::DockerContainer>("DockerContainer");
    qRegisterMetaType<amnezia::TransportProto>("TransportProto");
    qRegisterMetaType<amnezia::Proto>("Proto");
    qRegisterMetaType<amnezia::ServiceType>("ServiceType");
    qRegisterMetaType<amnezia::ErrorCode>("amnezia::ErrorCode");
    qRegisterMetaType<Vpn::ConnectionState>("Vpn::ConnectionState");
}

Result Context::reload()
{
    m_serversModel->resetModel();

    if (hasServers()) {
        const int defaultIndex = m_serversModel->getDefaultServerIndex();
        if (defaultIndex >= 0 && defaultIndex < m_serversModel->getServersCount()) {
            m_serversModel->setProcessedServerIndex(defaultIndex);
        }
    } else {
        m_containersModel->updateModel(QJsonArray {});
    }

    refreshActiveContainer();
    emit statusChanged();
    return Result::success();
}

bool Context::hasServers() const
{
    return m_serversModel->getServersCount() > 0;
}

Result Context::resolveServerIndexResult(int requestedIndex, int &resolvedIndex) const
{
    if (!hasServers()) {
        return Result::failure("No servers configured");
    }

    resolvedIndex = requestedIndex >= 0 ? requestedIndex : m_serversModel->getDefaultServerIndex();
    if (resolvedIndex < 0 || resolvedIndex >= m_serversModel->getServersCount()) {
        return Result::failure(QString("Server index %1 is out of range").arg(resolvedIndex));
    }

    return Result::success();
}

void Context::activateServer(int resolvedIndex, bool alsoSetDefault)
{
    m_serversModel->setProcessedServerIndex(resolvedIndex);
    if (alsoSetDefault) {
        m_serversModel->setDefaultServerIndex(resolvedIndex);
    }
    refreshActiveContainer();
}

void Context::refreshActiveContainer()
{
    if (m_activeServerIndex < 0 || m_activeServerIndex >= m_serversModel->getServersCount()) {
        m_activeContainer = amnezia::DockerContainer::None;
        return;
    }

    const auto current = m_serversModel->data(m_activeServerIndex, ServersModel::Roles::DefaultContainerRole);
    m_activeContainer = qvariant_cast<amnezia::DockerContainer>(current);
}

void Context::setLastError(amnezia::ErrorCode error)
{
    m_lastError = error;
    emit statusChanged();
}

void Context::clearLastError()
{
    m_lastError = amnezia::ErrorCode::NoError;
    emit statusChanged();
}

QJsonObject Context::serverSummary(int index) const
{
    QJsonObject server;
    server["index"] = index;
    server["name"] = m_serversModel->data(index, ServersModel::Roles::NameRole).toString();
    server["host"] = m_serversModel->data(index, ServersModel::Roles::HostNameRole).toString();
    server["description"] = m_serversModel->data(index, ServersModel::Roles::ServerDescriptionRole).toString();
    server["default"] = m_serversModel->data(index, ServersModel::Roles::IsDefaultRole).toBool();
    server["default_container"] = ContainerProps::containerToString(
            qvariant_cast<amnezia::DockerContainer>(m_serversModel->data(index, ServersModel::Roles::DefaultContainerRole)));
    server["default_container_name"] = displayContainerName(
            qvariant_cast<amnezia::DockerContainer>(m_serversModel->data(index, ServersModel::Roles::DefaultContainerRole)));
    server["has_write_access"] = m_serversModel->data(index, ServersModel::Roles::HasWriteAccessRole).toBool();
    server["has_installed_containers"] = m_serversModel->data(index, ServersModel::Roles::HasInstalledContainers).toBool();
    server["is_api"] = m_serversModel->data(index, ServersModel::Roles::IsServerFromTelegramApiRole).toBool()
                       || m_serversModel->data(index, ServersModel::Roles::IsServerFromGatewayApiRole).toBool();
    server["credentials_login"] = m_serversModel->data(index, ServersModel::Roles::CredentialsLoginRole).toString();
    return server;
}

QJsonObject Context::containerSummary(const QJsonObject &containerConfig, const QString &defaultContainerName) const
{
    const QString containerName = containerConfig.value(config_key::container).toString();
    const auto container = ContainerProps::containerFromString(containerName);

    QJsonObject summary;
    summary["container"] = containerName;
    summary["type"] = ContainerProps::containerTypeToString(container);
    summary["display_name"] = displayContainerName(container);
    summary["default"] = containerName == defaultContainerName;
    summary["supported"] = ContainerProps::isSupportedByCurrentPlatform(container);
    summary["service_type"] = serviceTypeKey(ContainerProps::containerService(container));

    QJsonArray protocols;
    for (const auto proto : ContainerProps::protocolsForContainer(container)) {
        protocols.append(ProtocolProps::protoToString(proto));
    }
    summary["protocols"] = protocols;

    return summary;
}

QJsonArray Context::containerSummaries(const QJsonObject &serverConfig) const
{
    const QString defaultContainerName = serverConfig.value(config_key::defaultContainer).toString();
    QJsonArray containers;
    for (const auto &value : serverConfig.value(config_key::containers).toArray()) {
        containers.append(containerSummary(value.toObject(), defaultContainerName));
    }
    return containers;
}

Result Context::listServers()
{
    reload();

    QJsonArray servers;
    for (int index = 0; index < m_serversModel->getServersCount(); ++index) {
        servers.append(serverSummary(index));
    }

    QJsonObject data;
    data["count"] = m_serversModel->getServersCount();
    data["servers"] = servers;

    return Result::success(m_serversModel->getServersCount() ? "Servers loaded" : "No servers configured", data);
}

Result Context::showServer(int requestedIndex)
{
    reload();

    int resolvedIndex = -1;
    Result resolved = resolveServerIndexResult(requestedIndex, resolvedIndex);
    if (!resolved.ok) {
        return resolved;
    }

    QJsonObject data;
    auto server = serverSummary(resolvedIndex);
    server["containers"] = containerSummaries(m_serversModel->getServerConfig(resolvedIndex));
    data["server"] = server;
    return Result::success("Server loaded", data);
}

Result Context::addServer(const QString &name, const amnezia::ServerCredentials &credentials)
{
    reload();

    if (!credentials.isValid()) {
        return Result::failure("Host, user, secret, and a valid SSH port are required");
    }

    QJsonObject server;
    server.insert(config_key::hostName, credentials.hostName);
    server.insert(config_key::userName, credentials.userName);
    server.insert(config_key::password, credentials.secretData);
    server.insert(config_key::port, credentials.port);
    server.insert(config_key::description, name.isEmpty() ? m_settings->nextAvailableServerName() : name);
    server.insert(config_key::defaultContainer, ContainerProps::containerToString(amnezia::DockerContainer::None));

    m_serversModel->addServer(server);
    reload();

    QJsonObject data;
    data["server"] = serverSummary(m_serversModel->getServersCount() - 1);
    return Result::success("Server added", data);
}

Result Context::importConfigFromFile(const QString &fileName)
{
    reload();

    Result result;
    QList<QMetaObject::Connection> connections;
    connections.append(QObject::connect(m_importController.get(), &ImportController::importFinished, this, [&result]() {
        result = Result::success("Configuration imported");
    }));
    connections.append(QObject::connect(m_importController.get(), &ImportController::importErrorOccurred, this, [&result](ErrorCode error, bool) {
        result = Result::failure(errorString(error));
    }));

    if (!m_importController->extractConfigFromFile(fileName)) {
        if (result.message.isEmpty()) {
            result = Result::failure("Failed to read configuration file");
        }
        disconnectTemporaryConnections(connections);
        return result;
    }

    m_importController->importConfig();
    reload();
    disconnectTemporaryConnections(connections);

    if (!result.ok) {
        return result;
    }

    QJsonObject data;
    data["count"] = m_serversModel->getServersCount();
    return Result::success(result.message, data);
}

Result Context::importConfigFromData(const QString &data)
{
    reload();

    Result result;
    QList<QMetaObject::Connection> connections;
    connections.append(QObject::connect(m_importController.get(), &ImportController::importFinished, this, [&result]() {
        result = Result::success("Configuration imported");
    }));
    connections.append(QObject::connect(m_importController.get(), &ImportController::importErrorOccurred, this, [&result](ErrorCode error, bool) {
        result = Result::failure(errorString(error));
    }));

    if (!m_importController->extractConfigFromData(data)) {
        if (result.message.isEmpty()) {
            result = Result::failure("Failed to parse configuration");
        }
        disconnectTemporaryConnections(connections);
        return result;
    }

    m_importController->importConfig();
    reload();
    disconnectTemporaryConnections(connections);

    if (!result.ok) {
        return result;
    }

    QJsonObject payload;
    payload["count"] = m_serversModel->getServersCount();
    return Result::success(result.message, payload);
}

Result Context::removeServer(int requestedIndex)
{
    reload();

    int resolvedIndex = -1;
    Result resolved = resolveServerIndexResult(requestedIndex, resolvedIndex);
    if (!resolved.ok) {
        return resolved;
    }

    const QString name = m_serversModel->data(resolvedIndex, ServersModel::Roles::NameRole).toString();
    m_serversModel->removeServer(resolvedIndex);
    reload();

    return Result::success(QString("Server '%1' removed").arg(name));
}

Result Context::setDefaultServer(int requestedIndex)
{
    reload();

    int resolvedIndex = -1;
    Result resolved = resolveServerIndexResult(requestedIndex, resolvedIndex);
    if (!resolved.ok) {
        return resolved;
    }

    m_serversModel->setDefaultServerIndex(resolvedIndex);
    m_serversModel->setProcessedServerIndex(resolvedIndex);
    reload();

    QJsonObject data;
    data["server"] = serverSummary(resolvedIndex);
    return Result::success(QString("Default server set to #%1").arg(resolvedIndex), data);
}

Result Context::scanServer(int requestedIndex)
{
    reload();

    int resolvedIndex = -1;
    Result resolved = resolveServerIndexResult(requestedIndex, resolvedIndex);
    if (!resolved.ok) {
        return resolved;
    }

    activateServer(resolvedIndex, false);

    Result result = Result::failure("Scan failed");
    QList<QMetaObject::Connection> connections;
    connections.append(QObject::connect(m_installController.get(), &InstallController::scanServerFinished, this, [&result](bool foundNewContainers) {
        result = Result::success(foundNewContainers ? "Containers added from server" : "No new containers found");
    }));
    connections.append(QObject::connect(m_installController.get(), &InstallController::installationErrorOccurred, this, [&result](ErrorCode error) {
        result = Result::failure(errorString(error));
    }));

    m_installController->scanServerForInstalledContainers();
    reload();
    disconnectTemporaryConnections(connections);

    if (!result.ok) {
        return result;
    }

    QJsonObject data;
    auto server = serverSummary(resolvedIndex);
    server["containers"] = containerSummaries(m_serversModel->getServerConfig(resolvedIndex));
    data["server"] = server;
    return Result::success(result.message, data);
}

Result Context::listCountries(int requestedIndex)
{
    reload();

    int resolvedIndex = -1;
    Result resolved = resolveServerIndexResult(requestedIndex, resolvedIndex);
    if (!resolved.ok) {
        return resolved;
    }

    activateServer(resolvedIndex, false);

    if (!m_serversModel->data(resolvedIndex, ServersModel::Roles::IsServerFromGatewayApiRole).toBool()) {
        return Result::failure("Country selection is supported only for gateway API profiles");
    }

    clearLastError();
    const bool refreshed = m_apiSettingsController->getAccountInfo(true);
    const ErrorCode refreshError = m_lastError;

    const auto serverConfig = m_serversModel->getServerConfig(resolvedIndex);
    const auto fallbackCountries = availableCountriesArray(serverConfig);
    const auto refreshedCountries = m_apiAccountInfoModel->getAvailableCountries();
    const auto countries = refreshed && !refreshedCountries.isEmpty() ? refreshedCountries : fallbackCountries;

    if (countries.isEmpty()) {
        return Result::failure(refreshed ? "This server does not expose selectable countries"
                                         : (refreshError == ErrorCode::NoError ? "Failed to load available countries"
                                                                               : errorString(refreshError)));
    }

    if (!refreshed) {
        clearLastError();
    }

    const auto issuedByCountry = refreshed ? issuedConfigsByCountry(m_apiAccountInfoModel->getIssuedConfigsInfo()) : QHash<QString, QJsonObject> {};
    const QString currentCountryCode = selectedCountryCode(serverConfig);

    QJsonArray countryItems;
    for (const auto &value : countries) {
        countryItems.append(countrySummary(value.toObject(), currentCountryCode, issuedByCountry));
    }

    QJsonObject data;
    data["server"] = serverSummary(resolvedIndex);
    data["countries"] = countryItems;
    data["count"] = countryItems.size();
    data["selected_country_code"] = currentCountryCode;
    data["selected_country_name"] = selectedCountryName(serverConfig);
    data["refreshed"] = refreshed;
    if (!refreshed && refreshError != ErrorCode::NoError) {
        data["refresh_error"] = static_cast<int>(refreshError);
        data["refresh_error_text"] = errorString(refreshError);
    }

    return Result::success(refreshed ? "Countries loaded" : "Countries loaded from cached server config", data);
}

Result Context::setCountry(int requestedIndex, const QString &countryCode)
{
    reload();

    const QString requestedCountryCode = countryCode.trimmed();
    if (requestedCountryCode.isEmpty()) {
        return Result::failure("Country code is required");
    }

    int resolvedIndex = -1;
    Result resolved = resolveServerIndexResult(requestedIndex, resolvedIndex);
    if (!resolved.ok) {
        return resolved;
    }

    activateServer(resolvedIndex, false);

    if (!m_serversModel->data(resolvedIndex, ServersModel::Roles::IsServerFromGatewayApiRole).toBool()) {
        return Result::failure("Country selection is supported only for gateway API profiles");
    }

    clearLastError();
    const bool refreshed = m_apiSettingsController->getAccountInfo(true);
    const ErrorCode refreshError = m_lastError;

    auto serverConfig = m_serversModel->getServerConfig(resolvedIndex);
    const auto countries = refreshed && !m_apiAccountInfoModel->getAvailableCountries().isEmpty()
                                   ? m_apiAccountInfoModel->getAvailableCountries()
                                   : availableCountriesArray(serverConfig);

    if (countries.isEmpty()) {
        return Result::failure(refreshed ? "This server does not expose selectable countries"
                                         : (refreshError == ErrorCode::NoError ? "Failed to load available countries"
                                                                               : errorString(refreshError)));
    }

    if (!refreshed) {
        clearLastError();
    }

    QString matchedCountryCode;
    QString matchedCountryName;
    for (const auto &value : countries) {
        const auto countryObject = value.toObject();
        const QString currentCode = countryObject.value(apiDefs::key::serverCountryCode).toString();
        if (currentCode.compare(requestedCountryCode, Qt::CaseInsensitive) == 0) {
            matchedCountryCode = currentCode;
            matchedCountryName = countryObject.value(apiDefs::key::serverCountryName).toString();
            break;
        }
    }

    if (matchedCountryCode.isEmpty()) {
        return Result::failure(QString("Country '%1' is not available for this server").arg(requestedCountryCode));
    }

    if (selectedCountryCode(serverConfig).compare(matchedCountryCode, Qt::CaseInsensitive) == 0) {
        QJsonObject data;
        data["server"] = serverSummary(resolvedIndex);
        data["selected_country_code"] = matchedCountryCode;
        data["selected_country_name"] = matchedCountryName;
        return Result::success(QString("Country is already set to %1").arg(matchedCountryName), data);
    }

    Result result = Result::failure(QString("Failed to change country to %1").arg(matchedCountryName));
    QList<QMetaObject::Connection> connections;
    connections.append(QObject::connect(m_apiConfigsController.get(), &ApiConfigsController::changeApiCountryFinished, this,
                                        [&result](const QString &message) { result = Result::success(message); }));
    connections.append(QObject::connect(m_apiConfigsController.get(), &ApiConfigsController::errorOccurred, this,
                                        [&result](ErrorCode error) { result = Result::failure(errorString(error)); }));

    if (!m_apiConfigsController->updateServiceFromGateway(resolvedIndex, matchedCountryCode, matchedCountryName)) {
        if (result.message.isEmpty()) {
            result = Result::failure(m_lastError == ErrorCode::NoError ? "Failed to change country" : errorString(m_lastError));
        }
        disconnectTemporaryConnections(connections);
        return result;
    }

    disconnectTemporaryConnections(connections);
    reload();
    serverConfig = m_serversModel->getServerConfig(resolvedIndex);

    QJsonObject data;
    data["server"] = serverSummary(resolvedIndex);
    data["selected_country_code"] = selectedCountryCode(serverConfig);
    data["selected_country_name"] = selectedCountryName(serverConfig);
    return Result::success(result.ok ? result.message : QString("Country changed to %1").arg(matchedCountryName), data);
}

Result Context::listContainers(int requestedIndex)
{
    reload();

    int resolvedIndex = -1;
    Result resolved = resolveServerIndexResult(requestedIndex, resolvedIndex);
    if (!resolved.ok) {
        return resolved;
    }

    QJsonObject data;
    data["server"] = serverSummary(resolvedIndex);
    data["containers"] = containerSummaries(m_serversModel->getServerConfig(resolvedIndex));
    data["count"] = data["containers"].toArray().size();
    return Result::success("Containers loaded", data);
}

Result Context::setDefaultContainer(int requestedIndex, amnezia::DockerContainer container)
{
    reload();

    if (container == amnezia::DockerContainer::None) {
        return Result::failure("Container name is required");
    }

    int resolvedIndex = -1;
    Result resolved = resolveServerIndexResult(requestedIndex, resolvedIndex);
    if (!resolved.ok) {
        return resolved;
    }

    m_serversModel->setDefaultContainer(resolvedIndex, container);
    reload();

    QJsonObject data;
    data["server"] = serverSummary(resolvedIndex);
    return Result::success(QString("Default container set to %1").arg(displayContainerName(container)), data);
}

Result Context::removeContainer(int requestedIndex, amnezia::DockerContainer container)
{
    reload();

    if (container == amnezia::DockerContainer::None) {
        return Result::failure("Container name is required");
    }

    int resolvedIndex = -1;
    Result resolved = resolveServerIndexResult(requestedIndex, resolvedIndex);
    if (!resolved.ok) {
        return resolved;
    }

    activateServer(resolvedIndex, false);
    m_containersModel->setProcessedContainerIndex(static_cast<int>(container));

    Result result = Result::failure("Failed to remove container");
    QList<QMetaObject::Connection> connections;
    connections.append(QObject::connect(m_installController.get(), &InstallController::removeProcessedContainerFinished, this,
                                        [&result](const QString &message) {
        result = Result::success(message);
    }));
    connections.append(QObject::connect(m_installController.get(), &InstallController::installationErrorOccurred, this,
                                        [&result](ErrorCode error) {
        result = Result::failure(errorString(error));
    }));

    m_installController->removeProcessedContainer();
    reload();
    disconnectTemporaryConnections(connections);
    return result;
}

Result Context::installServer(const QString &name, const amnezia::ServerCredentials &credentials, amnezia::DockerContainer container,
                              int containerPort, amnezia::TransportProto transport, const QString &keyPassphrase)
{
    reload();

    if (!credentials.isValid()) {
        return Result::failure("Host, user, secret, and a valid SSH port are required");
    }
    if (container == amnezia::DockerContainer::None) {
        return Result::failure("Container name is required");
    }

    m_installController->setShouldCreateServer(true);
    m_installController->setProcessedServerCredentials(hostWithOptionalPort(credentials), credentials.userName, credentials.secretData);

    Result result = Result::failure("Failed to install server");
    QList<QMetaObject::Connection> connections;
    connections.append(QObject::connect(m_installController.get(), &InstallController::installServerFinished, this, [&result](const QString &message) {
        result = Result::success(message);
    }));
    connections.append(QObject::connect(m_installController.get(), &InstallController::serverAlreadyExists, this, [&result](int index) {
        result = Result::failure(QString("Server already exists at index %1").arg(index));
    }));
    connections.append(QObject::connect(m_installController.get(), &InstallController::wrongInstallationUser, this,
                                        [&result](const QString &message) {
        result = Result::failure(message);
    }));
    connections.append(QObject::connect(m_installController.get(), &InstallController::installationErrorOccurred, this,
                                        [&result](ErrorCode error) {
        result = Result::failure(errorString(error));
    }));
    connections.append(QObject::connect(m_installController.get(), &InstallController::passphraseRequestStarted, this,
                                        [this, keyPassphrase]() { m_installController->setEncryptedPassphrase(keyPassphrase); }));

    m_installController->install(container, containerPort, transport);
    reload();
    disconnectTemporaryConnections(connections);

    if (!result.ok) {
        return result;
    }

    const int newIndex = m_serversModel->getServersCount() - 1;
    if (!name.isEmpty() && newIndex >= 0) {
        auto server = m_serversModel->getServerConfig(newIndex);
        server[config_key::description] = name;
        m_serversModel->editServer(server, newIndex);
        reload();
    }

    QJsonObject data;
    data["server"] = serverSummary(m_serversModel->getServersCount() - 1);
    return Result::success(result.message, data);
}

Result Context::installContainer(int requestedIndex, amnezia::DockerContainer container, int containerPort,
                                 amnezia::TransportProto transport, const QString &keyPassphrase)
{
    reload();

    if (container == amnezia::DockerContainer::None) {
        return Result::failure("Container name is required");
    }

    int resolvedIndex = -1;
    Result resolved = resolveServerIndexResult(requestedIndex, resolvedIndex);
    if (!resolved.ok) {
        return resolved;
    }

    activateServer(resolvedIndex, false);
    m_installController->setShouldCreateServer(false);

    Result result = Result::failure("Failed to install container");
    QList<QMetaObject::Connection> connections;
    connections.append(QObject::connect(m_installController.get(), &InstallController::installContainerFinished,
                                        this, [&result](const QString &message, bool) { result = Result::success(message); }));
    connections.append(QObject::connect(m_installController.get(), &InstallController::installationErrorOccurred, this,
                                        [&result](ErrorCode error) {
        result = Result::failure(errorString(error));
    }));
    connections.append(QObject::connect(m_installController.get(), &InstallController::passphraseRequestStarted, this,
                                        [this, keyPassphrase]() { m_installController->setEncryptedPassphrase(keyPassphrase); }));

    m_installController->install(container, containerPort, transport);
    reload();
    disconnectTemporaryConnections(connections);

    if (!result.ok) {
        return result;
    }

    QJsonObject data;
    auto server = serverSummary(resolvedIndex);
    server["containers"] = containerSummaries(m_serversModel->getServerConfig(resolvedIndex));
    data["server"] = server;
    return Result::success(result.message, data);
}

Result Context::startConnection(int requestedIndex)
{
    reload();

    if (m_state == Vpn::ConnectionState::Connected) {
        return Result::failure("A VPN connection is already active");
    }
    if (m_state == Vpn::ConnectionState::Preparing || m_state == Vpn::ConnectionState::Connecting
        || m_state == Vpn::ConnectionState::Disconnecting || m_state == Vpn::ConnectionState::Reconnecting) {
        return Result::failure("A VPN operation is already in progress");
    }

    int resolvedIndex = -1;
    Result resolved = resolveServerIndexResult(requestedIndex, resolvedIndex);
    if (!resolved.ok) {
        return resolved;
    }

    activateServer(resolvedIndex, true);
    m_activeServerIndex = resolvedIndex;
    refreshActiveContainer();
    clearLastError();
    m_totalReceivedBytes = 0;
    m_totalSentBytes = 0;

    m_connectionController->toggleConnection();

    QJsonObject data;
    data["server"] = serverSummary(resolvedIndex);
    data["status"] = status();
    return Result::success("Connection requested", data);
}

Result Context::stopConnection()
{
    if (m_state == Vpn::ConnectionState::Disconnected || m_state == Vpn::ConnectionState::Unknown) {
        return Result::success("Already disconnected", status());
    }

    clearLastError();
    m_connectionController->closeConnection();
    return Result::success("Disconnect requested", status());
}

Result Context::cleanupLogs()
{
    Logger::cleanUp();
    return Result::success("Logs cleaned");
}

QJsonObject Context::status() const
{
    QJsonObject json;
    json["state"] = connectionStateKey(m_state);
    json["state_text"] = VpnProtocol::textConnectionState(m_state);
    json["state_code"] = static_cast<int>(m_state);
    json["connected"] = m_state == Vpn::ConnectionState::Connected;
    json["in_progress"] = m_state == Vpn::ConnectionState::Preparing || m_state == Vpn::ConnectionState::Connecting
                          || m_state == Vpn::ConnectionState::Disconnecting || m_state == Vpn::ConnectionState::Reconnecting;
    json["last_error"] = static_cast<int>(m_lastError);
    json["last_error_text"] = errorString(m_lastError);
    json["remote_address"] = m_vpnConnection ? m_vpnConnection->remoteAddress() : QString();
    json["received_bytes_total"] = QString::number(m_totalReceivedBytes);
    json["sent_bytes_total"] = QString::number(m_totalSentBytes);
    json["active_server_index"] = m_activeServerIndex;
    json["active_container"] = ContainerProps::containerToString(m_activeContainer);
    json["active_container_name"] = displayContainerName(m_activeContainer);

    if (m_activeServerIndex >= 0 && m_activeServerIndex < m_serversModel->getServersCount()) {
        json["active_server"] = serverSummary(m_activeServerIndex);
    }

    return json;
}

} // namespace cli
