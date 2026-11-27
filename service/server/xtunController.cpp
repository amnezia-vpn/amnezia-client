#include "xtunController.h"

#include <QDebug>

#include <optional>
#include <string>
#include <vector>

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

    std::optional<libxtun_proxy_proto> proxyProto(XtunProto proto)
    {
        switch (proto) {
        case XtunProto::Awg: return LIBXTUN_PROXY_PROTO_AWG;
        case XtunProto::Xray: return LIBXTUN_PROXY_PROTO_XRAY;
        case XtunProto::None: return std::nullopt;
        }
        return std::nullopt;
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

XtunController::Proxy::~Proxy()
{
    libxtun_proxy_free(&handle);
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

    std::unique_ptr<Proxy> proxy;
    if (const ErrorCode error = createProxy(config, proxy); error != ErrorCode::NoError) {
        return error;
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
        qCritical() << "XtunController::up: failed to create tunnel:" << libxtun_strerror(error);
        return errorCode(error);
    }

    if (const ErrorCode error = attachProxy(std::move(proxy)); error != ErrorCode::NoError) {
        return error;
    }

    if (const int error = libxtun_tunnel_start(&m_tunnel); error != LIBXTUN_OK) {
        qCritical() << "XtunController::up: failed to start tunnel:" << libxtun_strerror(error);
        down();
        return errorCode(error);
    }

    return ErrorCode::NoError;
}

ErrorCode XtunController::switchProxy(const TunnelConfig &config)
{
    if (!m_tunnel.handle) {
        qWarning() << "XtunController::switchProxy: the tunnel is not running";
        return ErrorCode::InternalError;
    }

    std::unique_ptr<Proxy> proxy;
    if (const ErrorCode error = createProxy(config, proxy); error != ErrorCode::NoError) {
        down();
        return error;
    }

    return attachProxy(std::move(proxy));
}

void XtunController::down()
{
    if (m_tunnel.handle && m_proxy) {
        libxtun_tunnel_detach_proxy(&m_tunnel, &m_proxy->handle);
    }
    m_proxy.reset();
    libxtun_tunnel_free(&m_tunnel);
}

ErrorCode XtunController::createProxy(const TunnelConfig &config, std::unique_ptr<Proxy> &proxy)
{
    const auto proto = proxyProto(static_cast<XtunProto>(config.proto()));
    if (!proto) {
        qWarning() << "XtunController::createProxy: xtun does not support protocol" << config.proto();
        return ErrorCode::NotImplementedError;
    }

    if (!config.apps().isEmpty()) {
        qWarning() << "XtunController::createProxy: xtun does not support app split tunneling yet";
    }

    auto created = std::make_unique<Proxy>();
    created->controller = this;
    created->generation = m_proxyGeneration + 1;
    created->handle.loghandler = &XtunController::onLog;
    created->handle.statushandler = &XtunController::onStatus;
    created->handle.statushandler_priv = created.get();

    const std::vector<std::string> includeRoutes = toStdStrings(config.includeRoutes());
    const std::vector<std::string> excludeRoutes = toStdStrings(config.excludeRoutes());
    const std::vector<const char *> includeRoutesArgv = toArgv(includeRoutes);
    const std::vector<const char *> excludeRoutesArgv = toArgv(excludeRoutes);

    libxtun_proxy_opts proxyOpts {};
    proxyOpts.size = sizeof(proxyOpts);
    proxyOpts.includeRoutes = includeRoutesArgv.data();
    proxyOpts.excludeRoutes = excludeRoutesArgv.data();

    if (const int error = libxtun_proxy_new(&created->handle, *proto, config.config().toStdString().c_str(), &proxyOpts);
        error != LIBXTUN_OK) {
        qCritical() << "XtunController::createProxy: failed to create proxy:" << libxtun_strerror(error);
        return errorCode(error);
    }

    proxy = std::move(created);
    return ErrorCode::NoError;
}

ErrorCode XtunController::attachProxy(std::unique_ptr<Proxy> proxy)
{
    if (const int error = libxtun_tunnel_attach_proxy(&m_tunnel, &proxy->handle); error != LIBXTUN_OK) {
        qCritical() << "XtunController::attachProxy: failed to attach proxy:" << libxtun_strerror(error);
        down();
        return errorCode(error);
    }

    m_proxyGeneration = proxy->generation;
    m_proxy = std::move(proxy);
    return ErrorCode::NoError;
}

void XtunController::onLog(const char *line, void *controller)
{
    Q_UNUSED(controller)
    qInfo().noquote() << "xtun:" << line;
}

void XtunController::onStatus(int status, void *proxy)
{
    const auto *source = static_cast<Proxy *>(proxy);
    XtunController *self = source->controller;
    const quint64 generation = source->generation;
    QMetaObject::invokeMethod(
            self,
            [self, generation, status]() {
                if (generation == self->m_proxyGeneration) {
                    emit self->statusChanged(tunnelStatus(status));
                }
            },
            Qt::QueuedConnection);
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
