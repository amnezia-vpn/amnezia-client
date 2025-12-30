#include "xraycontroller.h"

#include "proxylogger.h"
#include "core/ipcclient.h"

#include <QFile>

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

bool XrayController::start(const QString &configPath)
{

    if (m_isRunning) {
        ProxyLogger::getInstance().info("Xray is already running");
        return true;
    }
    
    ProxyLogger::getInstance().info("Request to start Xray via IPC");

    m_lastError.clear();

    QString configContent;
    if (!loadConfigFile(configPath, configContent)) {
        return false;
    }

    const bool ipcResult = IpcClient::withInterface([&](QSharedPointer<IpcInterfaceReplica> iface) {
        iface->xrayStart(configContent);
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

bool XrayController::loadConfigFile(const QString &configPath, QString &configContent)
{
    if (configPath.isEmpty()) {
        m_lastError = "Config path is empty";
        ProxyLogger::getInstance().error(m_lastError);
        return false;
    }

    QFile configFile(configPath);
    if (!configFile.exists()) {
        m_lastError = QString("Config file not found at: %1").arg(configPath);
        ProxyLogger::getInstance().error(m_lastError);
        return false;
    }

    if (!configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_lastError = QString("Failed to open config file: %1").arg(configFile.errorString());
        ProxyLogger::getInstance().error(m_lastError);
        return false;
    }

    QByteArray data = configFile.readAll();
    configFile.close();

    if (data.isEmpty()) {
        m_lastError = QString("Config file is empty: %1").arg(configPath);
        ProxyLogger::getInstance().error(m_lastError);
        return false;
    }

    configContent = QString::fromUtf8(data);
    ProxyLogger::getInstance().debug(QString("Loaded Xray config from %1").arg(configPath));
    return true;
}