#include "xtunController.h"

#include <QDebug>

#include <optional>
#include <string>
#include <vector>

#include "core/utils/protocolEnum.h"

#ifdef Q_OS_MACOS
    #include <unistd.h>
#endif

using namespace amnezia;

namespace
{
    const std::vector<std::string> tunnelAddresses = { "172.19.0.1/30", "fdfe:dcba:9876::1/126" };

    std::vector<std::string> toStdStrings(const QStringList &list)
    {
        std::vector<std::string> result;
        result.reserve(list.size());
        for (const QString &item : list) {
            result.push_back(item.toStdString());
        }
        return result;
    }

    std::vector<const char *> toArgv(const std::vector<std::string> &items)
    {
        std::vector<const char *> argv;
        argv.reserve(items.size() + 1);
        for (const std::string &item : items) {
            argv.push_back(item.c_str());
        }
        argv.push_back(nullptr);
        return argv;
    }

    std::optional<libxtun_proxy_proto> proxyProto(Proto proto)
    {
        switch (proto) {
        case Proto::Awg:
        case Proto::WireGuard: return LIBXTUN_PROXY_PROTO_AWG;
        case Proto::Xray:
        case Proto::SSXray: return LIBXTUN_PROXY_PROTO_VLESS;
        default: return std::nullopt;
        }
    }

    libxtun_killswitch_mode xtunKillSwitchMode(KillSwitchMode mode)
    {
        switch (mode) {
        case KillSwitchMode::Soft: return LIBXTUN_KILLSWITCH_MODE_SOFT;
        case KillSwitchMode::Hard: return LIBXTUN_KILLSWITCH_MODE_HARD;
        case KillSwitchMode::Disabled: return LIBXTUN_KILLSWITCH_MODE_DISABLED;
        }
        return LIBXTUN_KILLSWITCH_MODE_DISABLED;
    }

    TunnelStatus tunnelStatus(int status)
    {
        switch (status) {
        case LIBXTUN_STATUS_CONNECTED: return TunnelStatus::Connected;
        case LIBXTUN_STATUS_NO_NETWORK: return TunnelStatus::NoNetwork;
        default: return TunnelStatus::Disconnected;
        }
    }

    ErrorCode errorCode(int xtunError)
    {
        return xtunError == LIBXTUN_ERR_UNSUPPORTED ? ErrorCode::NotImplementedError : ErrorCode::InternalError;
    }
}

XtunController::XtunController(QObject *parent) : QObject(parent)
{
}

XtunController::~XtunController()
{
    down();
}

ErrorCode XtunController::up(const TunnelConfig &config)
{
    down();

    const auto proto = proxyProto(static_cast<Proto>(config.proto()));
    if (!proto) {
        qWarning() << "XtunController::up: xtun does not support protocol" << config.proto();
        return ErrorCode::NotImplementedError;
    }

    if (!config.sites().isEmpty() || !config.apps().isEmpty()) {
        qWarning() << "XtunController::up: xtun does not support split tunneling yet";
    }

    const std::vector<std::string> dnsServers = toStdStrings(config.dnsServers());
    const std::vector<std::string> allowedDnsServers = toStdStrings(config.allowedDnsServers());
    const std::vector<const char *> dnsArgv = toArgv(dnsServers);
    const std::vector<const char *> allowedDnsArgv = toArgv(allowedDnsServers);
    const std::vector<const char *> tunnelAddressesArgv = toArgv(tunnelAddresses);

    libxtun_tunnel_opts opts {};
    opts.size = sizeof(opts);
    opts.dns = dnsArgv.data();
    opts.dns_exceptions = allowedDnsArgv.data();
    opts.killswitch_mode = xtunKillSwitchMode(static_cast<KillSwitchMode>(config.killSwitchMode()));
    opts.if_addrs = tunnelAddressesArgv.data();
    opts.mtu = config.mtu();
#ifdef Q_OS_MACOS
    opts.macos_group = static_cast<int>(::getegid());
#endif

    if (const int error = libxtun_tunnel_new(&m_tunnel, &opts); error != LIBXTUN_OK) {
        qCritical() << "XtunController::up: failed to create tunnel, xtun error" << error;
        return errorCode(error);
    }

    m_proxy.loghandler = &XtunController::onLog;
    m_proxy.loghandler_priv = this;
    m_proxy.statushandler = &XtunController::onStatus;
    m_proxy.statushandler_priv = this;

    if (const int error = libxtun_proxy_new(&m_proxy, *proto, config.config().toStdString().c_str());
        error != LIBXTUN_OK) {
        qCritical() << "XtunController::up: failed to create proxy, xtun error" << error;
        down();
        return errorCode(error);
    }

    if (const int error = libxtun_tunnel_attach_proxy(&m_tunnel, &m_proxy); error != LIBXTUN_OK) {
        qCritical() << "XtunController::up: failed to attach proxy, xtun error" << error;
        down();
        return errorCode(error);
    }

    if (const int error = libxtun_tunnel_start(&m_tunnel); error != LIBXTUN_OK) {
        qCritical() << "XtunController::up: failed to start tunnel, xtun error" << error;
        down();
        return errorCode(error);
    }

    return ErrorCode::NoError;
}

void XtunController::down()
{
    if (m_tunnel.handle && m_proxy.handle) {
        libxtun_tunnel_detach_proxy(&m_tunnel, &m_proxy);
    }
    libxtun_proxy_free(&m_proxy);
    m_proxy = {};
    libxtun_tunnel_free(&m_tunnel);
}

void XtunController::onLog(const char *line, void *controller)
{
    Q_UNUSED(controller)
    qInfo().noquote() << "xtun:" << line;
}

void XtunController::onStatus(int status, void *controller)
{
    auto *self = static_cast<XtunController *>(controller);
    QMetaObject::invokeMethod(
            self, [self, status]() { emit self->statusChanged(tunnelStatus(status)); }, Qt::QueuedConnection);
}

bool XtunController::setKillSwitchMode(KillSwitchMode mode)
{
    qWarning() << "XtunController::setKillSwitchMode: xtun cannot change killswitch mode at runtime yet, requested"
               << static_cast<int>(mode);
    return false;
}

bool XtunController::addKillSwitchAllowedRanges(const QStringList &ranges)
{
    qWarning()
            << "XtunController::addKillSwitchAllowedRanges: xtun does not support killswitch exceptions yet, requested"
            << ranges.size() << "ranges";
    return false;
}

bool XtunController::startLocalProxy(const QString &config)
{
    Q_UNUSED(config)
    qWarning() << "XtunController::startLocalProxy: xtun does not support local proxy mode yet";
    return false;
}

bool XtunController::stopLocalProxy()
{
    return true;
}

bool XtunController::isLocalProxyRunning() const
{
    return false;
}
