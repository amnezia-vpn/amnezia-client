// SPDX-License-Identifier: GPL-3.0-or-later
//
// Service-side singleton that hosts the native MasterDnsVPN engine in the
// privileged daemon process. Mirrors service/server/xray.{h,cpp} for the
// xray case — the GUI client speaks to us via the IPC slots
// `masterDnsVpnStart` / `masterDnsVpnStop` / `masterDnsVpnSocksPort`.
//
// The privileged-daemon address space is the right home for the engine
// because:
//
//   * It already binds the TUN device and owns route/DNS state, so the
//     SOCKS5 listener and tun2socks live in the same process and can be
//     wired together without an extra IPC hop.
//   * On Linux/macOS we may need elevated permissions to bind the engine's
//     outbound UDP sockets to a specific physical interface (so packets
//     don't loop back through the TUN). Doing that in the unprivileged
//     GUI process would require capability handoff; running here avoids
//     the problem.

#ifndef MASTER_DNS_VPN_SERVICE_H
#define MASTER_DNS_VPN_SERVICE_H

#include <QObject>
#include <QString>
#include <QtGlobal>
#include <memory>

namespace amnezia::masterdnsvpn {
class Engine;
}

class MasterDnsVpnService : public QObject
{
    Q_OBJECT

public:
    static MasterDnsVpnService &getInstance();

    // configJson is the same structured JSON the model emits via
    // MasterDnsVpnProtocolConfig::toJson() — see
    // client/core/models/protocols/masterDnsVpnProtocolConfig.h for the
    // schema.
    bool start(const QString &configJson);
    bool stop();

    // Returns 0 if the engine isn't currently listening (Idle / failed).
    quint16 socksPort() const;

private:
    MasterDnsVpnService();
    ~MasterDnsVpnService();
    Q_DISABLE_COPY_MOVE(MasterDnsVpnService)

    std::unique_ptr<amnezia::masterdnsvpn::Engine> m_engine;
};

#endif // MASTER_DNS_VPN_SERVICE_H
