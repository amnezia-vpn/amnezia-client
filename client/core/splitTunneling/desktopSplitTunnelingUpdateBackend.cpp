#include "desktopSplitTunnelingUpdateBackend.h"

#include <QJsonArray>
#include <QPointer>
#include <QRemoteObjectPendingCallWatcher>

#include "core/utils/constants/configKeys.h"
#include "core/utils/ipcClient.h"

using namespace amnezia;

void DesktopSplitTunnelingUpdateBackend::apply(quint64 requestId, const QJsonObject &configuration)
{
    if (!supportsConfiguration(configuration)) {
        emit finished(requestId, SplitTunnelingUpdateResult::NotApplied);
        return;
    }
    const QPointer<DesktopSplitTunnelingUpdateBackend> guard(this);
    IpcClient::withInterface(
            [guard, requestId, configuration](QSharedPointer<IpcInterfaceReplica> iface) {
                if (!guard) {
                    return;
                }
                const QString protocol = configuration.value(configKey::vpnProto).toString();
                const QString publicKey =
                        configuration.value(protocol + "_config_data").toObject().value(configKey::serverPubKey).toString();
                QStringList sites;
                for (const auto &site : configuration.value(configKey::splitTunnelSites).toArray()) {
                    sites.append(site.toString());
                }
                QRemoteObjectPendingReply<int> reply = iface->updateSplitTunneling(
                        configuration.value(configKey::splitTunnelType).toInt(), sites, publicKey,
                        configuration.value(configKey::splitTunnelSessionId).toString());
                if (reply.isFinished()) {
                    emit guard->finished(requestId,
                                         reply.error() == QRemoteObjectPendingCall::NoError
                                                 ? splitTunnelingUpdateResultFromInt(reply.returnValue())
                                                 : SplitTunnelingUpdateResult::StateUnknown);
                    return;
                }
                auto *watcher = new QRemoteObjectPendingCallWatcher(reply, guard);
                connect(watcher, &QRemoteObjectPendingCallWatcher::finished, guard, [guard, requestId, reply, watcher]() {
                    emit guard->finished(requestId,
                                         reply.error() == QRemoteObjectPendingCall::NoError
                                                 ? splitTunnelingUpdateResultFromInt(reply.returnValue())
                                                 : SplitTunnelingUpdateResult::StateUnknown);
                    watcher->deleteLater();
                });
            },
            [guard, requestId]() {
                if (guard) {
                    emit guard->finished(requestId, SplitTunnelingUpdateResult::NotApplied);
                }
            });
}
