import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "TextTypes"

Rectangle {
    id: root

    property string placeholderText
    property string text
    property string headerText
    property alias textArea: textArea
    property alias textAreaText: textArea.text

    property string borderHoveredColor: AmneziaStyle.color.charcoalGray
    property string borderNormalColor: AmneziaStyle.color.slateGray
    property string borderFocusedColor: AmneziaStyle.color.paleGray

    property string firstButtonImage
    property string secondButtonImage

    property var firstButtonClickedFunc
    property var secondButtonClickedFunc

    height: 148
    color: AmneziaStyle.color.onyxBlack
    border.width: 1
    border.color: getBorderColor(borderNormalColor)
    radius: 16

    property FlickableType parentFlickable: null
    onFocusChanged: {
        if (root.activeFocus) {
            if (root.parentFlickable) {
                root.parentFlickable.ensureVisible(root)
            }
        }
    }

    MouseArea {
        id: parentMouse
        anchors.fill: parent
        cursorShape: Qt.IBeamCursor
        onClicked: textArea.forceActiveFocus()
        hoverEnabled: true

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 0

            LabelTextType {
                Layout.fillWidth: true
                text: root.headerText
            }

            TextArea {
                id: textArea

                Layout.fillWidth: true
                Layout.fillHeight: true

                leftPadding: 0
                Layout.bottomMargin: 16

                color: AmneziaStyle.color.paleGray
                selectionColor:  AmneziaStyle.color.richBrown
                selectedTextColor: AmneziaStyle.color.paleGray
                placeholderTextColor: AmneziaStyle.color.mutedGray

                font.pixelSize: 16
                font.weight: Font.Medium
                font.family: "PT Root UI VF"

                placeholderText: root.placeholderText
                text: root.text

                onCursorVisibleChanged:  {
                    if (textArea.cursorVisible) {
                        fl.interactive = true
                    } else {
                        fl.interactive = false
                    }
                }

                wrapMode: Text.Wrap

                MouseArea {
                    id: textAreaMouse
                    anchors.fill: parent
                    acceptedButtons: Qt.RightButton
                    hoverEnabled: true
                    onClicked: {
                        fl.interactive = true
                        contextMenu.open()
                    }
                }

                onFocusChanged: {
                    root.border.color = getBorderColor(borderNormalColor)
                }

                ContextMenuType {
                    id: contextMenu
                    textObj: textArea
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: -8
                spacing: 0
                ImageButtonType {
                    id: firstButton
                    visible: root.firstButtonImage !== ""

                    imageColor: AmneziaStyle.color.paleGray

                    image: root.firstButtonImage
                    onClicked: function() {
                        if (root.firstButtonClickedFunc && typeof root.firstButtonClickedFunc === "function") {
                            root.firstButtonClickedFunc()
                        }
                    }
                }

                ImageButtonType {
                    id: secondButton
                    visible: root.secondButtonImage !== ""

                    imageColor: AmneziaStyle.color.paleGray

                    image: root.secondButtonImage
                    onClicked: function() {
                        if (root.secondButtonClickedFunc && typeof root.secondButtonClickedFunc === "function") {
                            root.secondButtonClickedFunc()
                        }
                    }
                }

                Item {
                    Layout.fillWidth: true
                }

                ImageButtonType {
                    id: resetButton
                    imageColor: AmneziaStyle.color.paleGray

                    visible: root.textAreaText !== ""
                    image: "qrc:/images/controls/close.svg"

                    onClicked: function() {
                        root.textAreaText = ""
                        textArea.focus = true
                    }
                }
            }
        }

        onPressed: {
            root.border.color = getBorderColor(borderFocusedColor)
        }

        onExited: {
            root.border.color = getBorderColor(borderNormalColor)
        }

        onEntered: {
            root.border.color = getBorderColor(borderHoveredColor)
        }
    }


    function getBorderColor(noneFocusedColor) {
        return textArea.focus ? root.borderFocusedColor : noneFocusedColor
    }
}
