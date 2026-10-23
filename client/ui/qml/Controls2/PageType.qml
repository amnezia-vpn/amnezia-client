import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "../Config"

Item {
    id: root

    property StackView stackView: StackView.view
    property bool enableTimer: true
    property bool isTabBarHidden: false

    Rectangle {
        anchors.fill: parent
        z: -1

        color: AmneziaStyle.color.bgBase
    }

    onVisibleChanged: {
        if (visible && enableTimer) {
            timer.start()
        }
    }

    // Set a timer to set focus after a short delay
    Timer {
        id: timer
        interval: 200 // Milliseconds
        onTriggered: {
            for (let item = root.Window.activeFocusItem; item; item = item.parent) {
                if (item === root.Overlay.overlay) {
                    return
                }
            }
            root.forceActiveFocus()
        }
        repeat: false // Stop the timer after one trigger
        running: enableTimer // Start the timer
    }
}
