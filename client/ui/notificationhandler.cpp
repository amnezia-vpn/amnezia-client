/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QDebug>
#include "notificationhandler.h"

#if defined(Q_OS_IOS)
#  include "platforms/ios/iosnotificationhandler.h"
#elif defined(Q_OS_ANDROID)
#  include "platforms/android/android_notificationhandler.h"
#else

#  include "systemtray_notificationhandler.h"
#endif

// static
NotificationHandler* NotificationHandler::create(QObject* parent) {
#if defined(Q_OS_IOS)
    return new IOSNotificationHandler(parent);
#elif defined(Q_OS_ANDROID)
    return new AndroidNotificationHandler(parent);
#else

#  if defined(Q_OS_LINUX)
    //if (LinuxSystemTrayNotificationHandler::requiredCustomImpl()) {
    //    return new LinuxSystemTrayNotificationHandler(parent);
    //}
#  endif

    return new SystemTrayNotificationHandler(parent);
#endif
}

namespace {
NotificationHandler* s_instance = nullptr;
}  // namespace

// static
NotificationHandler* NotificationHandler::instance() {
    Q_ASSERT(s_instance);
    return s_instance;
}

NotificationHandler::NotificationHandler(QObject* parent) : QObject(parent) {
    Q_ASSERT(!s_instance);
    s_instance = this;
}

NotificationHandler::~NotificationHandler() {
    Q_ASSERT(s_instance == this);
    s_instance = nullptr;
}

void NotificationHandler::setConnectionState(VpnProtocol::VpnConnectionState state)
{
    if (state != VpnProtocol::Connected && state != VpnProtocol::Disconnected) {
        return;
    }

    QString title;
    QString message;

    switch (state) {
    case VpnProtocol::VpnConnectionState::Connected:
        m_connected = true;

        title = tr("AmneziaVPN");
        message = tr("VPN Connected");
        break;

    case VpnProtocol::VpnConnectionState::Disconnected:
        if (m_connected) {
            m_connected = false;
            title = tr("AmneziaVPN");
            message = tr("VPN Disconnected");
        }
        break;

    default:
        break;
    }

    Q_ASSERT(title.isEmpty() == message.isEmpty());

    if (!title.isEmpty()) {
        notifyInternal(VpnState, title, message, 2000);
    }
}

void NotificationHandler::unsecuredNetworkNotification(const QString& networkName) {
    qDebug() << "Unsecured network notification shown";


    QString title = tr("AmneziaVPN notification");
    QString message = tr("Unsecured network detected: ") + networkName;

    notifyInternal(UnsecuredNetwork, title, message, 2000);
}

void NotificationHandler::notifyInternal(Message type, const QString& title,
                                         const QString& message,
                                         int timerMsec) {
    m_lastMessage = type;

    emit notificationShown(title, message);
    notify(type, title, message, timerMsec);
}

void NotificationHandler::messageClickHandle() {
    qDebug() << "Message clicked";

    if (m_lastMessage == VpnState) {
        qCritical() << "Random message clicked received";
        return;
    }

    emit notificationClicked(m_lastMessage);
    m_lastMessage = VpnState;
}
