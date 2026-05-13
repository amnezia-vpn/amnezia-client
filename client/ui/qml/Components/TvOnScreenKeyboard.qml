import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// FBLink VPN — D-pad navigable on-screen keyboard for Android TV.
//
// Android TV devices do not always expose a usable leanback IME, and the
// system OSK that *is* available tends to compete with QML's KeyNavigation
// for D-pad events. To keep the experience predictable we render our own
// keyboard inside the QML scene: a fixed grid of buttons that the user
// walks with the remote and confirms with OK.
//
// Public API:
//   - `value`            : current text being edited (read/write).
//   - `passwordMode`     : when true the value is rendered as bullets in
//                          consuming UI (this component doesn't mask, the
//                          parent does).
//   - `accepted()`       : emitted when the user activates the "Done" key.
//   - `dismissed()`      : emitted when the user activates the "Cancel"
//                          key or presses Back / Escape.
FocusScope {
    id: root

    property string value: ""
    property bool passwordMode: false
    property int maxLength: 256

    signal accepted()
    signal dismissed()

    implicitWidth: 920
    implicitHeight: 360

    function isOkKey(event) {
        return event.key === Qt.Key_Return
            || event.key === Qt.Key_Enter
            || event.key === Qt.Key_Select
    }

    function appendChar(ch) {
        if (root.value.length >= root.maxLength) {
            return
        }
        root.value = root.value + ch
    }

    function backspace() {
        if (root.value.length === 0) {
            return
        }
        root.value = root.value.slice(0, root.value.length - 1)
    }

    // ---- Layouts ---------------------------------------------------------
    //
    // Each row is a JS array of strings — single character keys plus
    // optional widened special keys whose codes are kept distinct so the
    // delegate can render them.
    QtObject {
        id: layouts
        readonly property var en: [
            ["1","2","3","4","5","6","7","8","9","0"],
            ["q","w","e","r","t","y","u","i","o","p"],
            ["a","s","d","f","g","h","j","k","l","-"],
            ["z","x","c","v","b","n","m",".","_","@"]
        ]
        readonly property var enShift: [
            ["!","\"","#","$","%","&","'","(",")","*"],
            ["Q","W","E","R","T","Y","U","I","O","P"],
            ["A","S","D","F","G","H","J","K","L","+"],
            ["Z","X","C","V","B","N","M",":","/","?"]
        ]
        readonly property var ru: [
            ["1","2","3","4","5","6","7","8","9","0"],
            ["й","ц","у","к","е","н","г","ш","щ","з"],
            ["ф","ы","в","а","п","р","о","л","д","ж"],
            ["я","ч","с","м","и","т","ь","б","ю","х"]
        ]
        readonly property var ruShift: [
            ["!","\"","№",";","%",":","?","*","(",")"],
            ["Й","Ц","У","К","Е","Н","Г","Ш","Щ","З"],
            ["Ф","Ы","В","А","П","Р","О","Л","Д","Ж"],
            ["Я","Ч","С","М","И","Т","Ь","Б","Ю","Х"]
        ]
    }

    property int layoutIndex: 0   // 0=en, 1=ru
    property bool shifted: false

    readonly property var activeRows: {
        if (layoutIndex === 0) return shifted ? layouts.enShift : layouts.en
        return shifted ? layouts.ruShift : layouts.ru
    }

    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_Back || event.key === Qt.Key_Escape) {
            root.dismissed()
            event.accepted = true
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: 22
        color: "#101013"
        border.width: 1
        border.color: "#27272A"
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 10

        // ---- Character grid ------------------------------------------
        GridLayout {
            id: grid
            Layout.fillWidth: true
            Layout.fillHeight: true
            columns: 10
            rowSpacing: 8
            columnSpacing: 8

            Repeater {
                id: keysRepeater
                model: 40

                TvKeyboardKey {
                    id: cell

                    readonly property int rowIndex: Math.floor(index / 10)
                    readonly property int colIndex: index % 10
                    readonly property string ch:
                        root.activeRows[rowIndex] ? root.activeRows[rowIndex][colIndex] : ""

                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredWidth: 1
                    Layout.preferredHeight: 1
                    text: cell.ch
                    // First row receives focus when the keyboard opens.
                    focus: cell.rowIndex === 0 && cell.colIndex === 0
                    KeyNavigation.up: cell.rowIndex === 0
                            ? null
                            : keysRepeater.itemAt(index - 10)
                    KeyNavigation.down: cell.rowIndex === 3
                            ? (cell.colIndex < 2 ? shiftKey
                               : cell.colIndex < 4 ? layoutKey
                               : cell.colIndex < 8 ? spaceKey
                               : cell.colIndex === 8 ? backspaceKey
                               : doneKey)
                            : keysRepeater.itemAt(index + 10)
                    KeyNavigation.left: cell.colIndex === 0
                            ? null
                            : keysRepeater.itemAt(index - 1)
                    KeyNavigation.right: cell.colIndex === 9
                            ? null
                            : keysRepeater.itemAt(index + 1)
                    onActivated: root.appendChar(cell.ch)
                }
            }
        }

        // ---- Function row -------------------------------------------
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 70
            spacing: 8

            TvKeyboardKey {
                id: shiftKey
                Layout.fillWidth: true
                Layout.preferredHeight: 70
                text: root.shifted ? "abc" : "ABC"
                emphasized: true
                KeyNavigation.up: keysRepeater.itemAt(30)
                KeyNavigation.right: layoutKey
                onActivated: root.shifted = !root.shifted
            }

            TvKeyboardKey {
                id: layoutKey
                Layout.fillWidth: true
                Layout.preferredHeight: 70
                text: root.layoutIndex === 0 ? "RU" : "EN"
                emphasized: true
                KeyNavigation.up: keysRepeater.itemAt(31)
                KeyNavigation.left: shiftKey
                KeyNavigation.right: spaceKey
                onActivated: {
                    root.layoutIndex = root.layoutIndex === 0 ? 1 : 0
                    root.shifted = false
                }
            }

            TvKeyboardKey {
                id: spaceKey
                Layout.preferredWidth: 280
                Layout.preferredHeight: 70
                text: "␣"
                KeyNavigation.up: keysRepeater.itemAt(34)
                KeyNavigation.left: layoutKey
                KeyNavigation.right: backspaceKey
                onActivated: root.appendChar(" ")
            }

            TvKeyboardKey {
                id: backspaceKey
                Layout.fillWidth: true
                Layout.preferredHeight: 70
                text: "⌫"
                KeyNavigation.up: keysRepeater.itemAt(38)
                KeyNavigation.left: spaceKey
                KeyNavigation.right: doneKey
                onActivated: root.backspace()
            }

            TvKeyboardKey {
                id: doneKey
                Layout.fillWidth: true
                Layout.preferredHeight: 70
                text: qsTr("Готово")
                emphasized: true
                accent: true
                KeyNavigation.up: keysRepeater.itemAt(39)
                KeyNavigation.left: backspaceKey
                onActivated: root.accepted()
            }
        }
    }
}
