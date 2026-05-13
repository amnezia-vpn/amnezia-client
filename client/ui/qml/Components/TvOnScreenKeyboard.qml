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

    implicitWidth: 920
    implicitHeight: 420

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
        // function row underneath. Layout: Shift(2) Lang/Sym(2) Space(2)
        // Sym/ABC(1) Backspace(1) Done(2).
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

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 10

        // ---- 4 rows × 10 keys ---------------------------------------
        //
        // We build the character grid as a vertical column of horizontal
        // rows so each row gets an equal slice of the available vertical
        // space (`Layout.fillHeight`), instead of all keys collapsing to
        // their preferred height (which is what a GridLayout produces
        // when every cell has `preferredHeight: 1`).
        Repeater {
            id: rowsRepeater
            model: 4

            RowLayout {
                id: keyRow
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: 1
                spacing: 8

                readonly property int rowIndex: index
                property alias cells: rowCells

                Repeater {
                    id: rowCells
                    model: 10

                    TvKeyboardKey {
                        readonly property int colIndex: index
                        readonly property int flatIndex:
                                keyRow.rowIndex * 10 + colIndex
                        readonly property string ch:
                                root.activeKeys[flatIndex] || ""

                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.preferredWidth: 1
                        text: ch
                        focus: keyRow.rowIndex === 0 && colIndex === 0
                        KeyNavigation.left: colIndex === 0
                                ? null
                                : rowCells.itemAt(colIndex - 1)
                        KeyNavigation.right: colIndex === 9
                                ? null
                                : rowCells.itemAt(colIndex + 1)
                        KeyNavigation.up: keyRow.rowIndex === 0
                                ? null
                                : rowsRepeater.itemAt(keyRow.rowIndex - 1)
                                        .cells.itemAt(colIndex)
                        KeyNavigation.down: keyRow.rowIndex === 3
                                ? root.functionTargetForColumn(colIndex)
                                : rowsRepeater.itemAt(keyRow.rowIndex + 1)
                                        .cells.itemAt(colIndex)
                        onActivated: root.appendChar(ch)
                    }
                }
            }
        }

        // ---- Function row -------------------------------------------
        RowLayout {
            id: functionRow
            Layout.fillWidth: true
            Layout.preferredHeight: 70
            spacing: 8

            TvKeyboardKey {
                id: shiftKey
                Layout.preferredWidth: 1
                Layout.fillWidth: true
                Layout.fillHeight: true
                text: root.shifted ? "abc" : "ABC"
                emphasized: true
                enabled: root.mode === TvOnScreenKeyboard.Mode.Letters
                KeyNavigation.right: langKey
                onActivated: root.shifted = !root.shifted
            }

            TvKeyboardKey {
                id: langKey
                Layout.preferredWidth: 1
                Layout.fillWidth: true
                Layout.fillHeight: true
                text: root.layoutIndex === 0 ? "RU" : "EN"
                emphasized: true
                enabled: root.mode === TvOnScreenKeyboard.Mode.Letters
                KeyNavigation.left: shiftKey
                KeyNavigation.right: spaceKey
                onActivated: {
                    root.layoutIndex = root.layoutIndex === 0 ? 1 : 0
                    root.shifted = false
                }
            }

            TvKeyboardKey {
                id: spaceKey
                Layout.preferredWidth: 2
                Layout.fillWidth: true
                Layout.fillHeight: true
                text: "␣"
                KeyNavigation.left: langKey
                KeyNavigation.right: symKey
                onActivated: root.appendChar(" ")
            }

            TvKeyboardKey {
                id: symKey
                Layout.preferredWidth: 1
                Layout.fillWidth: true
                Layout.fillHeight: true
                text: root.mode === TvOnScreenKeyboard.Mode.Symbols
                        ? "ABC"
                        : "?123"
                emphasized: true
                KeyNavigation.left: spaceKey
                KeyNavigation.right: backspaceKey
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
                Layout.preferredWidth: 1
                Layout.fillWidth: true
                Layout.fillHeight: true
                iconSource: "qrc:/images/controls/delete.svg"
                KeyNavigation.left: symKey
                KeyNavigation.right: doneKey
                onActivated: root.backspace()
            }

            TvKeyboardKey {
                id: doneKey
                Layout.preferredWidth: 1.4
                Layout.fillWidth: true
                Layout.fillHeight: true
                text: qsTr("Готово")
                emphasized: true
                accent: true
                KeyNavigation.left: backspaceKey
                onActivated: root.accepted()
            }
        }
    }
}
