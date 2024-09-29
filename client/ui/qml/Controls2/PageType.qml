import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../Config"

Item {
    id: root

    property StackView stackView: StackView.view

    onVisibleChanged: {
        if (visible && !GC.isMobile()) {
            timer.start()
        }
    }

    // Set a timer to set focus after a short delay
    Timer {
        id: timer
        interval: 500 // Milliseconds
        onTriggered: {
            FocusController.resetRootObject()
            FocusController.setFocusOnDefaultItem()
        }
        repeat: false // Stop the timer after one trigger
        running: !GC.isMobile()  // Start the timer
    }
}
