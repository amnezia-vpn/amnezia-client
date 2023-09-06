import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property StackView stackView: StackView.view

    MouseArea {
        z: 99
        anchors.fill: parent

        enabled: true

        onPressed: function(mouse) {
            forceActiveFocus()
            mouse.accepted = false
        }
    }
}
