#include "connectionController.h"

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS) || defined(MACOS_NE)
    #include <QGuiApplication>
#else
    #include <QApplication>
#endif

#include "utilities.h"
#include "core/controllers/vpnConfigurationController.h"
#include "version.h"
#include <QDateTime>
#include <QFileInfo>
#include <QSettings>

ConnectionController::ConnectionController(const QSharedPointer<ServersModel> &serversModel,
                                           const QSharedPointer<ContainersModel> &containersModel,
                                           const QSharedPointer<ClientManagementModel> &clientManagementModel,
                                           const QSharedPointer<VpnConnection> &vpnConnection, const std::shared_ptr<Settings> &settings,
                                           QObject *parent)
    : QObject(parent),
      m_serversModel(serversModel),
      m_containersModel(containersModel),
      m_clientManagementModel(clientManagementModel),
      m_vpnConnection(vpnConnection),
      m_settings(settings)
{
    connect(m_vpnConnection.get(), &VpnConnection::connectionStateChanged, this, &ConnectionController::onConnectionStateChanged);
    connect(this, &ConnectionController::connectToVpn, m_vpnConnection.get(), &VpnConnection::connectToVpn, Qt::QueuedConnection);
    connect(this, &ConnectionController::disconnectFromVpn, m_vpnConnection.get(), &VpnConnection::disconnectFromVpn, Qt::QueuedConnection);

    connect(this, &ConnectionController::connectButtonClicked, this, &ConnectionController::toggleConnection, Qt::QueuedConnection);

    m_state = Vpn::ConnectionState::Disconnected;
}

void ConnectionController::openConnection()
{
    QSettings qSettings(QSettings::NativeFormat, QSettings::UserScope, "FBLinkVPN", "FBLinkVPN");
    const qint64 safeModeUntilEpoch = qSettings.value("Conf/safeModeUntilEpochSec", 0).toLongLong();
    if (safeModeUntilEpoch > QDateTime::currentSecsSinceEpoch()) {
        qWarning() << "ConnectionController::openConnection: blocked by safe mode until" << safeModeUntilEpoch;
        emit m_vpnConnection->connectionStateChanged(Vpn::ConnectionState::Disconnected);
        return;
    }

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    // Let the first attempt through after m_state entered a terminal state
    // (Disconnected / Error / Unknown) so a legitimate retry after a failed
    // server switch is not swallowed by the 1200 ms throttle. The flag is
    // armed by onConnectionStateChanged() on terminal transitions and cleared
    // here on pass-through. Subsequent rapid taps fall back to the throttle,
    // which prevents two connectToVpn messages from stacking on the worker
    // thread before m_state updates to Connecting.
    const bool canBypassThrottle = m_retryPrimed
            && (m_state == Vpn::ConnectionState::Disconnected
                || m_state == Vpn::ConnectionState::Error
                || m_state == Vpn::ConnectionState::Unknown);
    if (!canBypassThrottle
        && m_state != Vpn::ConnectionState::Connected
        && (now - m_lastConnectAttemptMsec) < 1200) {
        qWarning() << "ConnectionController::openConnection: connect attempt throttled";
        return;
    }
    m_retryPrimed = false;
    m_lastConnectAttemptMsec = now;

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS) && !defined(MACOS_NE)
    // Check for FBLink-service OR the service named after the running EXE (e.g. FBLinkVPN-service)
    const QString altServiceName = QFileInfo(QCoreApplication::applicationFilePath()).baseName() + "-service";
    const bool serviceRunning =
        Utils::processIsRunning(Utils::executable(SERVICE_NAME, false), true) ||
        Utils::processIsRunning(Utils::executable(altServiceName, false), true);
    if (!serviceRunning)
    {
        emit connectionErrorOccurred(ErrorCode::FBLinkServiceNotRunning);
        return;
    }
#endif

    int serverIndex = m_serversModel->getDefaultServerIndex();
    QJsonObject serverConfig = m_serversModel->getServerConfig(serverIndex);

    DockerContainer container = qvariant_cast<DockerContainer>(m_serversModel->data(serverIndex, ServersModel::Roles::DefaultContainerRole));

    if (!m_containersModel->isSupportedByCurrentPlatform(container)) {
        emit connectionErrorOccurred(ErrorCode::NotSupportedOnThisPlatform);
        return;
    }

    QSharedPointer<ServerController> serverController(new ServerController(m_settings));
    VpnConfigurationsController vpnConfigurationController(m_settings, serverController);

    QJsonObject containerConfig = m_containersModel->getContainerConfig(container);
    ServerCredentials credentials = m_serversModel->getServerCredentials(serverIndex);

    auto dns = m_serversModel->getDnsPair(serverIndex);

    auto vpnConfiguration = vpnConfigurationController.createVpnConfiguration(dns, serverConfig, containerConfig, container);
    emit connectToVpn(serverIndex, credentials, container, vpnConfiguration);
}

void ConnectionController::closeConnection()
{
    emit disconnectFromVpn();
}

ErrorCode ConnectionController::getLastConnectionError()
{
    return m_vpnConnection->lastError();
}

void ConnectionController::onConnectionStateChanged(Vpn::ConnectionState state)
{
    const Vpn::ConnectionState previousState = m_state;
    m_state = state;

    // Prime the throttle bypass exactly on the edge into a terminal state so
    // openConnection()'s first attempt per entry gets through. Subsequent
    // attempts while still terminal stay throttled at 1200 ms, which prevents
    // rapid double-taps from stacking two connectToVpn messages on the worker
    // thread before the state updates to Connecting.
    const bool isTerminal = state == Vpn::ConnectionState::Disconnected
            || state == Vpn::ConnectionState::Error
            || state == Vpn::ConnectionState::Unknown;
    const bool wasTerminal = previousState == Vpn::ConnectionState::Disconnected
            || previousState == Vpn::ConnectionState::Error
            || previousState == Vpn::ConnectionState::Unknown;
    if (isTerminal && !wasTerminal) {
        m_retryPrimed = true;
    } else if (!isTerminal) {
        m_retryPrimed = false;
    }

    m_isConnected = false;
    m_connectionStateText = tr("Подключение...");
    switch (state) {
    case Vpn::ConnectionState::Connected: {
        m_isConnectionInProgress = false;
        m_isConnected = true;
        m_connectionStateText = tr("Подключено");
        break;
    }
    case Vpn::ConnectionState::Connecting: {
        m_isConnectionInProgress = true;
        break;
    }
    case Vpn::ConnectionState::Reconnecting: {
        m_isConnectionInProgress = true;
        m_connectionStateText = tr("Переподключение...");
        break;
    }
    case Vpn::ConnectionState::Disconnected: {
        m_isConnectionInProgress = false;
        m_connectionStateText = tr("Подключиться");
        break;
    }
    case Vpn::ConnectionState::Disconnecting: {
        m_isConnectionInProgress = true;
        m_connectionStateText = tr("Отключение...");
        break;
    }
    case Vpn::ConnectionState::Preparing: {
        m_isConnectionInProgress = true;
        m_connectionStateText = tr("Подготовка...");
        break;
    }
    case Vpn::ConnectionState::Error: {
        m_isConnectionInProgress = false;
        m_connectionStateText = tr("Подключиться");
        emit connectionErrorOccurred(getLastConnectionError());
        break;
    }
    case Vpn::ConnectionState::Unknown: {
        m_isConnectionInProgress = false;
        m_connectionStateText = tr("Подключиться");
        emit connectionErrorOccurred(getLastConnectionError());
        break;
    }
    }
    emit connectionStateChanged();
}

void ConnectionController::onTranslationsUpdated()
{
    // get translated text of current state
    onConnectionStateChanged(getCurrentConnectionState());
}

void ConnectionController::setConnectionStateText(const QString &text)
{
    m_connectionStateText = text;
    emit connectionStateChanged();
}

Vpn::ConnectionState ConnectionController::getCurrentConnectionState()
{
    return m_state;
}

QString ConnectionController::connectionStateText() const
{
    return m_connectionStateText;
}

void ConnectionController::toggleConnection()
{
    if (m_state == Vpn::ConnectionState::Preparing) {
        emit preparingConfig();
        return;
    }

    if (isConnectionInProgress()) {
        closeConnection();
    } else if (isConnected()) {
        closeConnection();
    } else {
        emit prepareConfig();
    }
}

bool ConnectionController::isConnectionInProgress() const
{
    return m_isConnectionInProgress;
}

bool ConnectionController::isConnected() const
{
    return m_isConnected;
}
