#include "networkReachabilityController.h"

#include <QNetworkInformation>

namespace {

    bool reachabilityAllowsRemoteOperations(QNetworkInformation::Reachability r) {
        using R = QNetworkInformation::Reachability;
        // Unknown: no backend or not yet determined — do not block UI.
        return r == R::Online || r == R::Unknown;
    }

} // namespace

NetworkReachabilityController::NetworkReachabilityController(QObject *parent) : QObject(parent) {
    attachToNetworkInformation();
}

bool NetworkReachabilityController::hasInternetAccess() const {
    return m_hasInternetAccess;
}

void NetworkReachabilityController::attachToNetworkInformation() {
    if (!QNetworkInformation::loadDefaultBackend()) {
        return;
    }
    QNetworkInformation *ni = QNetworkInformation::instance();
    if (!ni) {
        return;
    }
    const bool initial = reachabilityAllowsRemoteOperations(ni->reachability());
    const bool previous = m_hasInternetAccess;
    m_hasInternetAccess = initial;
    if (previous != m_hasInternetAccess) {
        emit hasInternetAccessChanged();
    }
    connect(ni, &QNetworkInformation::reachabilityChanged, this,
            [this](QNetworkInformation::Reachability r) {
                const bool ok = reachabilityAllowsRemoteOperations(r);
                if (ok == m_hasInternetAccess) {
                    return;
                }
                m_hasInternetAccess = ok;
                emit hasInternetAccessChanged();
            });
}
