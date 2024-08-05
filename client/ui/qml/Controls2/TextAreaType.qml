import QtQuick
import QtQuick.Controls

import Style 1.0

Rectangle {
    id: root

    property string placeholderText
    property string text
    property alias textArea: textArea
    property alias textAreaText: textArea.text

    property string borderHoveredColor: AmneziaStyle.color.greyDisabled
    property string borderNormalColor: AmneziaStyle.color.greyDark
    property string borderFocusedColor: AmneziaStyle.color.white

    height: 148
    color: AmneziaStyle.color.blackLight
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

        FlickableType {
            id: fl
            interactive: false

            anchors.top: parent.top
            anchors.bottom: parent.bottom
            contentHeight: textArea.implicitHeight
            TextArea {
                id: textArea

                width: parent.width

                topPadding: 16
                leftPadding: 16
                anchors.topMargin: 16
                anchors.bottomMargin: 16

                color: AmneziaStyle.color.white
                selectionColor:  AmneziaStyle.color.brown
                selectedTextColor: AmneziaStyle.color.white
                placeholderTextColor: AmneziaStyle.color.grey

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
