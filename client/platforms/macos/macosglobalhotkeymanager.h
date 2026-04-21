/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef MACOSGLOBALHOTKEYMANAGER_H
#define MACOSGLOBALHOTKEYMANAGER_H

#include <Carbon/Carbon.h>
#include <QObject>

class MacOSGlobalHotkeyManager final : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(MacOSGlobalHotkeyManager)

public:
    explicit MacOSGlobalHotkeyManager(QObject *parent = nullptr);
    ~MacOSGlobalHotkeyManager() override;

    bool registerHotKey(quint32 keyCode, quint32 modifiers);
    void unregisterHotKey();

signals:
    void activated();

private:
    EventHotKeyRef m_hotKeyRef = nullptr;
    EventHandlerRef m_eventHandlerRef = nullptr;
};

#endif // MACOSGLOBALHOTKEYMANAGER_H
