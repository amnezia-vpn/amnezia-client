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
    if (isRunning()) {
        return true;
    }

    m_lastError.clear();
    m_token = 0;

    if (configJson.trimmed().isEmpty()) {
        m_lastError = QStringLiteral("Config content is empty");
        qCWarning(lcLocalProxy) << m_lastError;
        return false;
    }

    const qint64 token = IpcClient::withInterface(
            [&](QSharedPointer<IpcInterfaceReplica> iface) -> qint64 {
                auto xrayStart = iface->xrayStartOwned(configJson);
                if (!xrayStart.waitForFinished()) {
                    return 0;
                }
                return xrayStart.returnValue();
            },
            []() -> qint64 { return 0; });

    if (token == 0) {
        m_lastError = kIpcUnavailableError;
        qCWarning(lcLocalProxy) << "Failed to start Xray via IPC";
        return false;
    }

    m_token = token;
    return true;
}

bool XrayEngineClient::stop()
{
    if (m_token == 0) {
        return true;
    }

    const bool stopped = IpcClient::withInterface(
            [this](QSharedPointer<IpcInterfaceReplica> iface) {
                auto xrayStop = iface->xrayStopOwned(m_token);
                return xrayStop.waitForFinished() && xrayStop.returnValue();
            },
            []() { return false; });

    if (!stopped) {
        m_lastError = kIpcUnavailableError;
        qCWarning(lcLocalProxy) << "Failed to stop Xray via IPC";
        return false;
    }

    m_token = 0;
    return true;
}

bool XrayEngineClient::isRunning() const
{
    if (m_token == 0) {
        return false;
    }

    return IpcClient::withInterface(
            [this](QSharedPointer<IpcInterfaceReplica> iface) {
                auto currentToken = iface->xrayCurrentToken();
                return currentToken.waitForFinished() && currentToken.returnValue() == m_token;
            },
            []() { return false; });
}

QString XrayEngineClient::lastError() const
{
    return m_lastError;
}
