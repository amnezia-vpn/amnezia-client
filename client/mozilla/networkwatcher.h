/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef NETWORKWATCHER_H
#define NETWORKWATCHER_H

#include <QElapsedTimer>
#include <QMap>
#include <QNetworkInformation>


class NetworkWatcherImpl;

// This class watches for network changes to detect unsecured wifi.
class NetworkWatcher final : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(NetworkWatcher)

public:
    NetworkWatcher();
    ~NetworkWatcher();

    void initialize();

    // Public for the Inspector.
    void unsecuredNetwork(const QString& networkName, const QString& networkId);
    // Used for the Inspector. simulateOffline = true to mock being disconnected,
    // false to restore.
    void simulateDisconnection(bool simulatedDisconnection);

    QNetworkInformation::Reachability getReachability();

signals:
    void networkChange();

private:
    void settingsChanged();

private:
    bool m_active = false;
    bool m_reportUnsecuredNetwork = false;

    // Platform-specific implementation.
    NetworkWatcherImpl* m_impl = nullptr;

    QMap<QString, QElapsedTimer> m_networks;

    // This is used to connect NotificationHandler lazily.
    bool m_firstNotification = true;

    // Used to simulate network disconnection in the Inspector
    bool m_simulatedDisconnection = false;
};

#endif  // NETWORKWATCHER_H
