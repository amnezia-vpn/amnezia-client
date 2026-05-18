#ifndef NETWORKREACHABILITYCONTROLLER_H
#define NETWORKREACHABILITYCONTROLLER_H

#include <QObject>

// Exposes QNetworkInformation to QML for UI that must not run remote operations offline.
// Note: mozilla/networkwatcher.h has NetworkWatcher::getReachability() using the same API,
// but networkwatcher.cpp is not linked into the desktop client (only the service process).

class NetworkReachabilityController final : public QObject {
Q_OBJECT

    Q_PROPERTY(bool hasInternetAccess READ hasInternetAccess NOTIFY hasInternetAccessChanged)

public:
    explicit NetworkReachabilityController(QObject *parent = nullptr);

    bool hasInternetAccess() const;

signals:

    void hasInternetAccessChanged();

private:
    void attachToNetworkInformation();

    bool m_hasInternetAccess = true;
};

#endif // NETWORKREACHABILITYCONTROLLER_H
