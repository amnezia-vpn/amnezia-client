#include "xraycontroller.h"

#include "proxylogger.h"
#include "core/ipcclient.h"

namespace {
const QString kIpcUnavailableError = QStringLiteral("Failed to communicate with IPC service");
}

XrayController::XrayController(QObject *parent)
    : QObject(parent)
{
    ProxyLogger::getInstance().debug("XrayController initialized");
}

XrayController::~XrayController()
{
    stop();
}

bool XrayController::start(const QString &configJson)
{
    if (m_isRunning) {
        ProxyLogger::getInstance().info("Xray is already running");
        return true;
    }

    ProxyLogger::getInstance().info("Request to start Xray via IPC");

    m_lastError.clear();

    if (configJson.trimmed().isEmpty()) {
        m_lastError = QStringLiteral("Config content is empty");
        ProxyLogger::getInstance().error(m_lastError);
        return false;
    }

    const bool ipcResult = IpcClient::withInterface([&](QSharedPointer<IpcInterfaceReplica> iface) {
        iface->xrayStart(configJson);
        return true;
    }, []() {
        return false;
    });

    if (!ipcResult) {
        m_lastError = kIpcUnavailableError;
        ProxyLogger::getInstance().error(m_lastError);
        return false;
    }

    ProxyLogger::getInstance().info("Xray start command sent to IPC service");
    m_isRunning = true;
    return true;
}

bool XrayController::stop()
{
    ProxyLogger::getInstance().info("Stopping Xray via IPC");

    const bool ipcResult = IpcClient::withInterface([](QSharedPointer<IpcInterfaceReplica> iface) {
        iface->xrayStop();
        return true;
    }, []() {
        return false;
    });

    if (!ipcResult) {
        m_lastError = kIpcUnavailableError;
        ProxyLogger::getInstance().warning(m_lastError);
        return false;
    }

    m_isRunning = false;
    return true;
}

bool XrayController::isXrayRunning() const
{
    return m_isRunning;
}

qint64 XrayController::getProcessId() const
{
    return -1;
}

QString XrayController::getError() const
{
    return m_lastError;
}