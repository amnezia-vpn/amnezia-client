#include "appleSplitTunnelingUpdateBackend.h"

#include <QPointer>

#include "core/utils/constants/configKeys.h"
#include "platforms/ios/ios_controller.h"

bool AppleSplitTunnelingUpdateBackend::supportsConfiguration(const QJsonObject &configuration) const
{
    const auto mode = configuration.value(amnezia::configKey::splitTunnelType).toInt();
    return (mode == 1 || mode == 2) && SplitTunnelingUpdateBackend::supportsConfiguration(configuration);
}

void AppleSplitTunnelingUpdateBackend::apply(quint64 requestId, const QJsonObject &configuration)
{
    if (!supportsConfiguration(configuration)) {
        emit finished(requestId, SplitTunnelingUpdateResult::NotApplied);
        return;
    }
    const QPointer<AppleSplitTunnelingUpdateBackend> guard(this);
    IosController::Instance()->updateSplitTunneling(requestId, configuration,
                                                    [guard, requestId](SplitTunnelingUpdateResult result) {
                                                        if (guard) {
                                                            emit guard->finished(requestId, result);
                                                        }
                                                    });
}
