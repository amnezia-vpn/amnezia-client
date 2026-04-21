/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "macosglobalhotkeymanager.h"
#include "logger.h"

namespace
{
    Logger logger("MacOSGlobalHotkeyManager");
    constexpr OSType hotKeySignature = 'AMZK';
    constexpr UInt32 hotKeyIdentifier = 1;

    OSStatus onHotKeyPressed(EventHandlerCallRef, EventRef event, void *userData)
    {
        if (GetEventClass(event) != kEventClassKeyboard || GetEventKind(event) != kEventHotKeyPressed) {
            return eventNotHandledErr;
        }

        EventHotKeyID hotKeyId {};
        GetEventParameter(event, kEventParamDirectObject, typeEventHotKeyID, nullptr, sizeof(hotKeyId), nullptr, &hotKeyId);

        auto *manager = static_cast<MacOSGlobalHotkeyManager *>(userData);
        logger.debug() << "Hotkey pressed:" << QString::number(hotKeyId.signature, 16) << hotKeyId.id;
        QMetaObject::invokeMethod(
            manager,
            [manager]() {
                logger.debug() << "Dispatching hotkey activation into Qt event loop";
                emit manager->activated();
            },
            Qt::QueuedConnection);
        return noErr;
    }
}

MacOSGlobalHotkeyManager::MacOSGlobalHotkeyManager(QObject *parent)
    : QObject(parent)
{
    const EventTypeSpec eventType = { kEventClassKeyboard, kEventHotKeyPressed };
    const OSStatus status = InstallApplicationEventHandler(&onHotKeyPressed, 1, &eventType, this, &m_eventHandlerRef);
    if (status != noErr) {
        logger.error() << "Failed to install application hotkey handler, status:" << static_cast<uint64_t>(status);
    } else {
        logger.debug() << "Application hotkey handler installed";
    }
}

MacOSGlobalHotkeyManager::~MacOSGlobalHotkeyManager()
{
    logger.debug() << "Destroying manager";
    unregisterHotKey();

    if (m_eventHandlerRef) {
        RemoveEventHandler(m_eventHandlerRef);
        m_eventHandlerRef = nullptr;
        logger.debug() << "Application hotkey handler removed";
    }
}

bool MacOSGlobalHotkeyManager::registerHotKey(quint32 keyCode, quint32 modifiers)
{
    unregisterHotKey();

    EventHotKeyID hotKeyId {};
    hotKeyId.signature = hotKeySignature;
    hotKeyId.id = hotKeyIdentifier;

    const OSStatus status = RegisterEventHotKey(static_cast<UInt32>(keyCode), static_cast<UInt32>(modifiers),
                                                hotKeyId, GetApplicationEventTarget(), 0, &m_hotKeyRef);
    logger.debug() << "Register hotkey request keyCode:" << static_cast<uint64_t>(keyCode)
                   << "modifiers:" << static_cast<uint64_t>(modifiers)
                   << "status:" << static_cast<uint64_t>(status)
                   << "registered:" << (status == noErr);
    return status == noErr;
}

void MacOSGlobalHotkeyManager::unregisterHotKey()
{
    if (m_hotKeyRef) {
        logger.debug() << "Unregistering current hotkey";
        UnregisterEventHotKey(m_hotKeyRef);
        m_hotKeyRef = nullptr;
    }
}
