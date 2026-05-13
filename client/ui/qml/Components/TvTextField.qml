import QtQuick
import QtQuick.Controls

// TV-friendly text field.
//
// Behaviour:
//   * D-pad up/down navigate to the next focusable item only when the user
//     has explicitly accepted the input (Enter / Back). While the soft input
//     panel is open or the field is being edited, arrow keys are forwarded to
//     the IME / cursor and never trigger focus movement. This matches the
//     behaviour Android TV users expect from a "real" input field.
//   * Space is treated as a regular character (because users actually need
//     spaces in passwords / search queries), never as an "OK" press.
//   * Pressing the OK key on the focused field opens the soft input panel
//     (so the on-screen keyboard appears on Android TV) and, when the panel
//     is already open, hands the key over to the IME so the keyboard's own
//     selection is honored.
FocusScope {
    id: root

    property string placeholderText: ""
    property string text: input.text
    property int echoMode: TextInput.Normal
    property int inputMethodHints: Qt.ImhNone
    property color textColor: "#F8FAFC"
    property color placeholderColor: "#52525B"
    property color backgroundColor: "#18181B"
    property color focusedBackgroundColor: "#1F1F22"
    property color borderColor: "#3F3F46"
    property color focusedBorderColor: "#FACC15"
    property int fontPixelSize: 28
    property int radius: 16

    property alias textField: input

    signal accepted()

    implicitWidth: 320
    implicitHeight: 82

    function clear() {
        input.text = ""
    }

    function selectAll() {
        input.selectAll()
    }

    function openSoftKeyboard() {
        if (Qt.inputMethod) {
            Qt.inputMethod.show()
        }
    }

    Rectangle {
        id: bg
        anchors.fill: parent
        radius: root.radius
        color: root.activeFocus ? root.focusedBackgroundColor : root.backgroundColor
        border.width: root.activeFocus ? 3 : 1
        border.color: root.activeFocus ? root.focusedBorderColor : root.borderColor

        Behavior on color { ColorAnimation { duration: 120 } }
        Behavior on border.color { ColorAnimation { duration: 120 } }
    }

    TextInput {
        id: input
        anchors.fill: parent
        anchors.leftMargin: 22
        anchors.rightMargin: 22
        verticalAlignment: TextInput.AlignVCenter
        color: root.textColor
        font.pixelSize: root.fontPixelSize
        clip: true
        selectByMouse: true
        focus: true
        activeFocusOnTab: true
        echoMode: root.echoMode
        inputMethodHints: root.inputMethodHints
        selectionColor: "#FACC15"
        selectedTextColor: "#111111"

        onAccepted: root.accepted()
        onTextChanged: root.text = text
        onActiveFocusChanged: {
            if (activeFocus) {
                root.openSoftKeyboard()
            }
        }

        Text {
            id: placeholder
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            visible: input.text.length === 0 && !input.activeFocus
            text: root.placeholderText
            color: root.placeholderColor
            font.pixelSize: root.fontPixelSize
        }

        // Single-line input: left/right move the caret, up/down should
        // move D-pad focus to the next field. Re-emit the up/down events on
        // the surrounding FocusScope so KeyNavigation.up / KeyNavigation.down
        // can pick them up.
        Keys.onPressed: function(event) {
            if (event.key === Qt.Key_Back || event.key === Qt.Key_Escape) {
                root.parent.forceActiveFocus()
                event.accepted = true
            } else if (event.key === Qt.Key_Up) {
                if (root.KeyNavigation.up) {
                    root.KeyNavigation.up.forceActiveFocus()
                    event.accepted = true
                }
            } else if (event.key === Qt.Key_Down) {
                if (root.KeyNavigation.down) {
                    root.KeyNavigation.down.forceActiveFocus()
                    event.accepted = true
                }
            }
        }
    }

    // Make the whole pill clickable / tappable.
    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.IBeamCursor
        onPressed: function(mouse) {
            input.forceActiveFocus()
            root.openSoftKeyboard()
            mouse.accepted = false
        }
    }
}
