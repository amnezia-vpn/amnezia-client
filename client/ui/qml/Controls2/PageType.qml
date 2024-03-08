import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property StackView stackView: StackView.view

    property var defaultActiveFocusItem: null

//    MouseArea {
//        id: globalMouseArea
//        z: 99
//        anchors.fill: parent

//        enabled: true

//        onPressed: function(mouse) {
//            forceActiveFocus()
//            mouse.accepted = false
//        }
//    }

    // Set a timer to set focus after a short delay
    Timer {
        id: timer
        interval: 100 // Milliseconds
        onTriggered: {
            if (defaultActiveFocusItem) {
                defaultActiveFocusItem.forceActiveFocus()
            }
        }
        repeat: false // Stop the timer after one trigger
        running: true // Start the timer
    }
}
