#include "appleSplitTunnelingUpdateBackend.h"

#include <QPointer>

#include "platforms/ios/ios_controller.h"

void AppleSplitTunnelingUpdateBackend::apply(quint64 requestId, const QJsonObject &configuration)
{
    if (!supportsConfiguration(configuration)) {
        emit finished(requestId, SplitTunnelingUpdateResult::NotApplied);
        return;
    }
    const QPointer<AppleSplitTunnelingUpdateBackend> guard(this);
    IosController::Instance()->updateSplitTunneling(configuration, [guard, requestId](SplitTunnelingUpdateResult result) {
        if (guard) {
            emit guard->finished(requestId, result);
        }
    });
}
