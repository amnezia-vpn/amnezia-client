import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import JsonSyntaxHighlighter 1.0
import Style 1.0

import "../Controls2"
import "../Config"

Rectangle {
    id: root

    property alias text: textArea.text
    property alias textArea: textArea
    property bool readOnly: false
    property int errorLine: 0
    property string logTag: "XrayCodeEditor"

    signal edited(string text)

    readonly property var lineStarts: {
        const starts = [0]
        const value = textArea.text
        for (let i = 0; i < value.length; ++i) {
            if (value.charAt(i) === "\n") {
                starts.push(i + 1)
            }
        }
        return starts
    }

    implicitHeight: Math.min(textArea.implicitHeight + 2, 560)

    radius: 16
    color: AmneziaStyle.color.bgSurface
    border.width: textArea.activeFocus ? 1 : 0
    border.color: AmneziaStyle.color.borderEmphasis

    Component.onCompleted: console.log("[XrayUI][" + root.logTag + "] created, readOnly", root.readOnly, "lines", root.lineStarts.length)
    onErrorLineChanged: console.log("[XrayUI][" + root.logTag + "] errorLine", root.errorLine)

    Flickable {
        id: flickable

        anchors.fill: parent
        anchors.margins: 1

        clip: true
        boundsBehavior: Flickable.StopAtBounds
        contentHeight: textArea.implicitHeight

        ScrollBar.vertical: ScrollBarType {}

        TextArea.flickable: TextArea {
            id: textArea

            leftPadding: gutter.width + 12
            rightPadding: 12
            topPadding: 16
            bottomPadding: 16

            readOnly: root.readOnly
            selectByMouse: true
            persistentSelection: true
            textFormat: TextEdit.PlainText
            wrapMode: TextEdit.Wrap
            inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText | Qt.ImhPreferLatin

            color: AmneziaStyle.color.textPrimary
            selectionColor: AmneziaStyle.color.accentDeep
            selectedTextColor: AmneziaStyle.color.textPrimary

            font.family: GC.monospaceFontFamily
            font.pixelSize: 13

            background: null

            onTextChanged: {
                if (textArea.activeFocus) {
                    console.log("[XrayUI][" + root.logTag + "] edited, length", textArea.length, "lines", root.lineStarts.length)
                    root.edited(textArea.text)
                }
            }

            onActiveFocusChanged: console.log("[XrayUI][" + root.logTag + "] focus", textArea.activeFocus)

            Item {
                id: gutter

                x: 0
                y: 0
                width: 12 + numberMetrics.advanceWidth * Math.max(2, String(root.lineStarts.length).length)
                height: textArea.height

                TextMetrics {
                    id: numberMetrics

                    font: textArea.font
                    text: "0"
                }

                Repeater {
                    model: root.lineStarts

                    Text {
                        required property int index
                        required property int modelData

                        x: 0
                        y: {
                            textArea.contentHeight
                            textArea.width
                            return modelData <= textArea.length ? textArea.positionToRectangle(modelData).y : 0
                        }
                        width: gutter.width - 4

                        horizontalAlignment: Text.AlignRight
                        text: index + 1
                        font: textArea.font
                        color: index + 1 === root.errorLine ? AmneziaStyle.color.statusDestructive
                                                            : AmneziaStyle.color.textQuaternary
                    }
                }
            }

            JsonSyntaxHighlighter {
                textDocument: textArea.textDocument
                valueColor: AmneziaStyle.color.accentDefault
                errorColor: AmneziaStyle.color.statusDestructive
                errorLine: root.errorLine
            }
        }
    }
}
