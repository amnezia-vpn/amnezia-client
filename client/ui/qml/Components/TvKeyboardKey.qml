import QtQuick
import QtQuick.Controls

// FBLink VPN — single key inside the TV on-screen keyboard. The key is just
// a focusable button that reports activations via the `activated` signal so
// the parent keyboard can decide what to do with it.
FocusScope {
    id: control

    property string text: ""
    property bool emphasized: false
    property bool accent: false

    signal activated()

    implicitWidth: 80
    implicitHeight: 70
    activeFocusOnTab: true

    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_Return
            || event.key === Qt.Key_Enter
            || event.key === Qt.Key_Select) {
            control.activated()
            event.accepted = true
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: 12
        color: control.accent
                ? (control.activeFocus ? "#FACC15" : "#A16207")
                : control.emphasized
                    ? (control.activeFocus ? "#3F2E07" : "#1F1F22")
                    : (control.activeFocus ? "#2A2104" : "#18181B")
        border.width: control.activeFocus ? 3 : 1
        border.color: control.activeFocus
                ? "#FACC15"
                : (control.emphasized ? "#3F3F46" : "#2A2A2D")

        Behavior on color { ColorAnimation { duration: 110 } }
        Behavior on border.color { ColorAnimation { duration: 110 } }
    }

    Text {
        anchors.centerIn: parent
        text: control.text
        color: control.accent
                ? (control.activeFocus ? "#111111" : "#FAFAFA")
                : "#F8FAFC"
        font.pixelSize: control.emphasized || control.accent ? 22 : 26
        font.bold: control.activeFocus || control.emphasized || control.accent
    }

    scale: control.activeFocus ? 1.06 : 1.0
    Behavior on scale {
        NumberAnimation { duration: 110; easing.type: Easing.OutQuad }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            control.forceActiveFocus()
            control.activated()
        }
    }
}
