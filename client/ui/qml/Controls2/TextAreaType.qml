import QtQuick
import QtQuick.Controls

Rectangle {
    id: root

    property string placeholderText
    property string text
    property alias textArea: textArea
    property alias textAreaText: textArea.text

    property string borderHoveredColor: "#494B50"
    property string borderNormalColor: "#2C2D30"
    property string borderFocusedColor: "#d7d8db"

    height: 148
    color: "#1C1D21"
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

                color: "#D7D8DB"
                selectionColor:  "#633303"
                selectedTextColor: "#D7D8DB"
                placeholderTextColor: "#878B91"

                font.pixelSize: 16
                font.weight: Font.Medium
                font.family: "Noto Sans"

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
