import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "../Config"

Item {
    id: root

    property StackView stackView: StackView.view

    Rectangle {
        anchors.fill: parent
        z: -2
        color: FBLinkStyle.color.midnightBlack
    }

    onVisibleChanged: {
        if (visible) {
            timer.start()
        }
    }

    // Set a timer to set focus after a short delay
    Timer {
        id: timer
        interval: 200 // Milliseconds
        onTriggered: {
            if (SettingsController.isTvInterfaceActive) {
                // On Android TV the TV FocusScope (PageTvRoot) owns the focus
                // chain; pulling focus back to the mobile defaultFocusItem
                // would eat the D-pad keys.
                return
            }
            FocusController.resetRootObject()
            FocusController.setFocusOnDefaultItem()
        }
        repeat: false // Stop the timer after one trigger
        running: true // Start the timer
    }
}
