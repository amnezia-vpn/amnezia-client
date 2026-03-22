import QtQuick
import QtQuick.Controls

import Style 1.0

Rectangle {
    id: root

    property string placeholderText
    property string text
    property alias textArea: textArea
    property alias textAreaText: textArea.text

    property string borderHoveredColor: FBLinkStyle.color.charcoalGray
    property string borderNormalColor: FBLinkStyle.color.slateGray
    property string borderFocusedColor: FBLinkStyle.color.paleGray

    height: 148
    color: FBLinkStyle.color.onyxBlack
    border.width: 1
    border.color: getBorderColor(borderNormalColor)
    radius: 16

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

                property bool isFocusable: true

                Keys.onTabPressed: {
                    FocusController.nextKeyTabItem()
                }

                Keys.onBacktabPressed: {
                    FocusController.previousKeyTabItem()
                }

                Keys.onUpPressed: {
                    FocusController.nextKeyUpItem()
                }
                
                Keys.onDownPressed: {
                    FocusController.nextKeyDownItem()
                }
                
                Keys.onLeftPressed: {
                    FocusController.nextKeyLeftItem()
                }

                Keys.onRightPressed: {
                    FocusController.nextKeyRightItem()
                }

                color: FBLinkStyle.color.paleGray
                selectionColor:  FBLinkStyle.color.richBrown
                selectedTextColor: FBLinkStyle.color.paleGray
                placeholderTextColor: FBLinkStyle.color.mutedGray

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

                ContextMenu.menu: ContextMenuType {
                    textObj: textArea
                }

                onFocusChanged: {
                    root.border.color = getBorderColor(borderNormalColor)
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
