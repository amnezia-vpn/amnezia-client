pragma Singleton
import QtQuick

import QtCore

QtObject {
    id: root

    property var pendingCallback: null

    Connections {
        target: Qt.application
        function onStateChanged() {
            if (pendingCallback && Qt.application.state === Qt.ApplicationActive) {
                var cb = pendingCallback
                pendingCallback = null
                Qt.callLater(cb)
            }
        }
    }

    function runWhenActive(callback) {
        if (!callback || typeof callback !== "function")
            return

        if (Qt.platform.os !== "android") {
            Qt.callLater(callback)
            return
        }

        if (Qt.application.state === Qt.ApplicationActive) {
            Qt.callLater(callback)
        } else {
            pendingCallback = callback
        }
    }
}
