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
//   - `dismissed()`      : emitted when the user presses Back / Escape.
FocusScope {
    id: root

    property string value: ""
    property bool passwordMode: false
    property int maxLength: 256

    signal accepted()
    signal dismissed()

    // Each character row is `rowHeight` tall, the function row is the same
    // (so all buttons look the same size). Five rows + four spacings +
    // outer margins are the source of truth for `implicitHeight`.
    readonly property int rowHeight: 72
    readonly property int rowSpacingValue: 8
    readonly property int outerMargin: 16

    implicitWidth: 960
    implicitHeight: 5 * rowHeight + 4 * rowSpacingValue + 2 * outerMargin

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
    // Each layout is a flat array of exactly 40 characters arranged in 4
    // rows of 10. Single-character codes only; widened special keys live
    // in the dedicated function row below.
    QtObject {
        id: layouts
        readonly property var en: [
            "1","2","3","4","5","6","7","8","9","0",
            "q","w","e","r","t","y","u","i","o","p",
            "a","s","d","f","g","h","j","k","l","-",
            "z","x","c","v","b","n","m",".","_","@"
        ]
        readonly property var enShift: [
            "1","2","3","4","5","6","7","8","9","0",
            "Q","W","E","R","T","Y","U","I","O","P",
            "A","S","D","F","G","H","J","K","L","-",
            "Z","X","C","V","B","N","M",".","_","@"
        ]
        readonly property var ru: [
            "1","2","3","4","5","6","7","8","9","0",
            "й","ц","у","к","е","н","г","ш","щ","з",
            "ф","ы","в","а","п","р","о","л","д","ж",
            "я","ч","с","м","и","т","ь","б","ю","х"
        ]
        readonly property var ruShift: [
            "1","2","3","4","5","6","7","8","9","0",
            "Й","Ц","У","К","Е","Н","Г","Ш","Щ","З",
            "Ф","Ы","В","А","П","Р","О","Л","Д","Ж",
            "Я","Ч","С","М","И","Т","Ь","Б","Ю","Х"
        ]
        readonly property var symbols: [
            "1","2","3","4","5","6","7","8","9","0",
            "!","@","#","$","%","&","*","+","-","=",
            "(",")","[","]","{","}","<",">","/","\\",
            "?",",",".",";",":","'","\"","_","`","~"
        ]
    }

    // ---- Modes -----------------------------------------------------------
    enum Mode {
        Letters,
        Symbols
    }

    property int layoutIndex: 0   // 0=en, 1=ru — for letters mode only.
    property bool shifted: false
    property int mode: TvOnScreenKeyboard.Mode.Letters

    readonly property var activeKeys: {
        if (mode === TvOnScreenKeyboard.Mode.Symbols) {
            return layouts.symbols
        }
        if (layoutIndex === 0) return shifted ? layouts.enShift : layouts.en
        return shifted ? layouts.ruShift : layouts.ru
    }

    function functionTargetForColumn(col) {
        // Map a column in the bottom-most character row to a key in the
        // function row underneath. Function row columns:
        //   0-1: shift, 2-3: lang, 4-5: space, 6: sym, 7: backspace,
        //   8-9: done.
        if (col < 2) return shiftKey
        if (col < 4) return langKey
        if (col < 6) return spaceKey
        if (col < 7) return symKey
        if (col < 8) return backspaceKey
        return doneKey
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

    // ---- Five fixed rows ------------------------------------------------
    //
    // We avoid `Repeater` for the rows themselves: Repeater children
    // inside a ColumnLayout don't always honour `Layout.fillHeight`, and
    // we want the keyboard to look identical to a hardcoded 5-row grid.
    // Each row uses an internal Repeater to instantiate its 10 cells.
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: root.outerMargin
        spacing: root.rowSpacingValue

        // Row 0
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: root.rowHeight
            spacing: root.rowSpacingValue

            Repeater {
                id: row0
                model: 10
                TvKeyboardKey {
                    readonly property int colIndex: index
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    Layout.preferredHeight: root.rowHeight
                    text: root.activeKeys[index] || ""
                    focus: index === 0
                    KeyNavigation.left: index === 0
                            ? null : row0.itemAt(index - 1)
                    KeyNavigation.right: index === 9
                            ? null : row0.itemAt(index + 1)
                    KeyNavigation.down: row1.itemAt(index)
                    onActivated: root.appendChar(root.activeKeys[index] || "")
                }
            }
        }

        // Row 1
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: root.rowHeight
            spacing: root.rowSpacingValue

            Repeater {
                id: row1
                model: 10
                TvKeyboardKey {
                    readonly property int colIndex: index
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    Layout.preferredHeight: root.rowHeight
                    text: root.activeKeys[10 + index] || ""
                    KeyNavigation.left: index === 0
                            ? null : row1.itemAt(index - 1)
                    KeyNavigation.right: index === 9
                            ? null : row1.itemAt(index + 1)
                    KeyNavigation.up: row0.itemAt(index)
                    KeyNavigation.down: row2.itemAt(index)
                    onActivated: root.appendChar(root.activeKeys[10 + index] || "")
                }
            }
        }

        // Row 2
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: root.rowHeight
            spacing: root.rowSpacingValue

            Repeater {
                id: row2
                model: 10
                TvKeyboardKey {
                    readonly property int colIndex: index
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    Layout.preferredHeight: root.rowHeight
                    text: root.activeKeys[20 + index] || ""
                    KeyNavigation.left: index === 0
                            ? null : row2.itemAt(index - 1)
                    KeyNavigation.right: index === 9
                            ? null : row2.itemAt(index + 1)
                    KeyNavigation.up: row1.itemAt(index)
                    KeyNavigation.down: row3.itemAt(index)
                    onActivated: root.appendChar(root.activeKeys[20 + index] || "")
                }
            }
        }

        // Row 3
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: root.rowHeight
            spacing: root.rowSpacingValue

            Repeater {
                id: row3
                model: 10
                TvKeyboardKey {
                    readonly property int colIndex: index
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    Layout.preferredHeight: root.rowHeight
                    text: root.activeKeys[30 + index] || ""
                    KeyNavigation.left: index === 0
                            ? null : row3.itemAt(index - 1)
                    KeyNavigation.right: index === 9
                            ? null : row3.itemAt(index + 1)
                    KeyNavigation.up: row2.itemAt(index)
                    KeyNavigation.down: root.functionTargetForColumn(index)
                    onActivated: root.appendChar(root.activeKeys[30 + index] || "")
                }
            }
        }

        // Function row
        RowLayout {
            id: functionRow
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: root.rowHeight
            spacing: root.rowSpacingValue

            TvKeyboardKey {
                id: shiftKey
                Layout.fillWidth: true
                Layout.preferredWidth: 2
                Layout.preferredHeight: root.rowHeight
                text: root.shifted ? "abc" : "ABC"
                emphasized: true
                enabled: root.mode === TvOnScreenKeyboard.Mode.Letters
                KeyNavigation.right: langKey
                KeyNavigation.up: row3.itemAt(0)
                onActivated: root.shifted = !root.shifted
            }

            TvKeyboardKey {
                id: langKey
                Layout.fillWidth: true
                Layout.preferredWidth: 2
                Layout.preferredHeight: root.rowHeight
                text: root.layoutIndex === 0 ? "RU" : "EN"
                emphasized: true
                enabled: root.mode === TvOnScreenKeyboard.Mode.Letters
                KeyNavigation.left: shiftKey
                KeyNavigation.right: spaceKey
                KeyNavigation.up: row3.itemAt(2)
                onActivated: {
                    root.layoutIndex = root.layoutIndex === 0 ? 1 : 0
                    root.shifted = false
                }
            }

            TvKeyboardKey {
                id: spaceKey
                Layout.fillWidth: true
                Layout.preferredWidth: 2
                Layout.preferredHeight: root.rowHeight
                text: "␣"
                KeyNavigation.left: langKey
                KeyNavigation.right: symKey
                KeyNavigation.up: row3.itemAt(4)
                onActivated: root.appendChar(" ")
            }

            TvKeyboardKey {
                id: symKey
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                Layout.preferredHeight: root.rowHeight
                text: root.mode === TvOnScreenKeyboard.Mode.Symbols
                        ? "ABC"
                        : "?123"
                emphasized: true
                KeyNavigation.left: spaceKey
                KeyNavigation.right: backspaceKey
                KeyNavigation.up: row3.itemAt(6)
                onActivated: {
                    root.mode =
                            root.mode === TvOnScreenKeyboard.Mode.Symbols
                                    ? TvOnScreenKeyboard.Mode.Letters
                                    : TvOnScreenKeyboard.Mode.Symbols
                    root.shifted = false
                }
            }

            TvKeyboardKey {
                id: backspaceKey
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                Layout.preferredHeight: root.rowHeight
                iconSource: "qrc:/images/controls/delete.svg"
                KeyNavigation.left: symKey
                KeyNavigation.right: doneKey
                KeyNavigation.up: row3.itemAt(7)
                onActivated: root.backspace()
            }

            TvKeyboardKey {
                id: doneKey
                Layout.fillWidth: true
                Layout.preferredWidth: 2
                Layout.preferredHeight: root.rowHeight
                text: qsTr("Готово")
                emphasized: true
                accent: true
                KeyNavigation.left: backspaceKey
                KeyNavigation.up: row3.itemAt(8)
                onActivated: root.accepted()
            }
        }
    }
}
