#include "xrayEngineClient.h"

#include "core/utils/ipcClient.h"
#include "localProxyDefs.h"

namespace
{
    const QString kIpcUnavailableError = QStringLiteral("Failed to communicate with IPC service");
}

XrayEngineClient::~XrayEngineClient()
{
    stop();
}

bool XrayEngineClient::start(const QString &configJson)
{
    if (m_isRunning) {
        return true;
    }

    m_lastError.clear();

    if (configJson.trimmed().isEmpty()) {
        m_lastError = QStringLiteral("Config content is empty");
        qCWarning(lcLocalProxy) << m_lastError;
        return false;
    }

    const bool started = IpcClient::withInterface(
            [&](QSharedPointer<IpcInterfaceReplica> iface) {
                auto xrayStart = iface->xrayStart(configJson);
                return xrayStart.waitForFinished() && xrayStart.returnValue();
            },
            []() { return false; });

    if (!started) {
        m_lastError = kIpcUnavailableError;
        qCWarning(lcLocalProxy) << "Failed to start Xray via IPC";
        return false;
    }

    m_isRunning = true;
    return true;
}

bool XrayEngineClient::stop()
{
    if (!m_isRunning) {
        return true;
    }

    const bool stopped = IpcClient::withInterface(
            [](QSharedPointer<IpcInterfaceReplica> iface) {
                auto xrayStop = iface->xrayStop();
                return xrayStop.waitForFinished() && xrayStop.returnValue();
            },
            []() { return false; });

    if (!stopped) {
        m_lastError = kIpcUnavailableError;
        qCWarning(lcLocalProxy) << "Failed to stop Xray via IPC";
        return false;
    }

    m_isRunning = false;
    return true;
}

bool XrayEngineClient::isRunning() const
{
    return m_isRunning;
}

QString XrayEngineClient::lastError() const
{
    return m_lastError;
}
