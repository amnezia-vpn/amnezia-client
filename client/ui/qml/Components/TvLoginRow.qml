import QtQuick
import QtQuick.Controls

// FBLink VPN — focusable row that displays a single value (email / password)
// and asks the parent to open an in-app TV keyboard when the user presses OK.
//
// The row never owns a Qt TextInput — that's intentional. On Android TV the
// remote interacts much more predictably with a custom QML keyboard panel
// (see `TvOnScreenKeyboard`) than with the system IME, and using a TextInput
// here would only re-introduce the "D-pad navigates between fields" bug.
FocusScope {
    id: control

    property string title: ""
    property string placeholder: ""
    property string value: ""
    property bool passwordMode: false

    signal activated()

    implicitWidth: 320
    implicitHeight: 78
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
        radius: 16
        color: control.activeFocus ? "#1F1F22" : "#18181B"
        border.width: control.activeFocus ? 3 : 1
        border.color: control.activeFocus ? "#FACC15" : "#3F3F46"

        Behavior on color { ColorAnimation { duration: 120 } }
        Behavior on border.color { ColorAnimation { duration: 120 } }
    }

    Item {
        anchors.fill: parent
        anchors.leftMargin: 22
        anchors.rightMargin: 22

        Label {
            id: titleLabel
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.topMargin: 8
            text: control.title
            color: "#A1A1AA"
            font.pixelSize: 18
        }

        Label {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 10
            verticalAlignment: Text.AlignBottom
            text: control.value.length === 0
                    ? control.placeholder
                    : (control.passwordMode
                            ? "•".repeat(control.value.length)
                            : control.value)
            color: control.value.length === 0 ? "#52525B" : "#F8FAFC"
            font.pixelSize: 26
            elide: Text.ElideRight
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            control.forceActiveFocus()
            control.activated()
        }
    }
}
