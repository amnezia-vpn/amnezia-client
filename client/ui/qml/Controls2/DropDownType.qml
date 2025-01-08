import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "TextTypes"
import "../Config"

Item {
    id: root

    property string text
    property string textColor: AmneziaStyle.color.paleGray
    property string textDisabledColor: AmneziaStyle.color.mutedGray
    property int textMaximumLineCount: 2
    property int textElide: Qt.ElideRight

    property string descriptionText
    property string descriptionTextColor: AmneziaStyle.color.mutedGray
    property string descriptionTextDisabledColor: AmneziaStyle.color.charcoalGray

    property string headerText
    property string headerBackButtonImage

    property var rootButtonClickedFunction
    property string rootButtonImage: "qrc:/images/controls/chevron-down.svg"
    property string rootButtonImageColor: AmneziaStyle.color.paleGray
    property string rootButtonBackgroundColor: AmneziaStyle.color.onyxBlack
    property string rootButtonBackgroundHoveredColor: AmneziaStyle.color.onyxBlack
    property string rootButtonBackgroundPressedColor: AmneziaStyle.color.onyxBlack

    property string borderFocusedColor: AmneziaStyle.color.paleGray
    property int borderFocusedWidth: 1

    property string rootButtonHoveredBorderColor: AmneziaStyle.color.charcoalGray
    property string rootButtonDefaultBorderColor: AmneziaStyle.color.slateGray
    property string rootButtonPressedBorderColor: AmneziaStyle.color.paleGray

    property int rootButtonTextLeftMargins: 16
    property int rootButtonTextTopMargin: 16
    property int rootButtonTextBottomMargin: 16

    property real drawerHeight: 0.9
    property Item drawerParent
    property Component listView

    signal openTriggered
    signal closeTriggered

    readonly property bool isFocusable: true

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

    implicitWidth: rootButtonContent.implicitWidth
    implicitHeight: rootButtonContent.implicitHeight

    onOpenTriggered: {
        menu.openTriggered()
    }

    onCloseTriggered: {
        menu.closeTriggered()
    }

    Keys.onEnterPressed: {
        if (menu.isClosed) {
            menu.openTriggered()
        }
    }

    Keys.onReturnPressed: {
        if (menu.isClosed) {
            menu.openTriggered()
        }
    }

    Rectangle {
        id: focusBorder

        color: AmneziaStyle.color.transparent
        border.color: root.activeFocus ? root.borderFocusedColor : AmneziaStyle.color.transparent
        border.width: root.activeFocus ? root.borderFocusedWidth : 0
        anchors.fill: rootButtonContent
        radius: 16


        Rectangle {
            id: rootButtonBackground

            anchors.fill: focusBorder
            anchors.margins: root.activeFocus ? 2 : 0
            radius: root.activeFocus ? 14 : 16

            color: {
                if (root.enabled) {
                    if (root.pressed) {
                        return root.rootButtonBackgroundPressedColor
                    }
                    return root.hovered ? root.rootButtonBackgroundHoveredColor : root.rootButtonBackgroundColor
                } else {
                    return AmneziaStyle.color.transparent
                }
            }

            border.color: rootButtonDefaultBorderColor
            border.width: 1

            Behavior on border.color {
                PropertyAnimation { duration: 200 }
            }

            Behavior on color {
                PropertyAnimation { duration: 200 }
            }
        }
    }

    RowLayout {
        id: rootButtonContent
        anchors.fill: parent

        spacing: 0

        ColumnLayout {
            Layout.leftMargin: rootButtonTextLeftMargins
            Layout.topMargin: rootButtonTextTopMargin
            Layout.bottomMargin: rootButtonTextBottomMargin

            LabelTextType {
                Layout.fillWidth: true

                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter

                visible: root.descriptionText !== ""

                color: root.enabled ? root.descriptionTextColor : root.descriptionTextDisabledColor
                text: root.descriptionText
            }

            ButtonTextType {
                id: buttonText
                Layout.fillWidth: true

                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter

                color: root.enabled ? root.textColor : root.textDisabledColor
                text: root.text
                maximumLineCount: root.textMaximumLineCount
                elide: root.textElide
            }
        }

        ImageButtonType {
            Layout.rightMargin: 16

            implicitWidth: 40
            implicitHeight: 40

            hoverEnabled: false
            image: rootButtonImage
            imageColor: rootButtonImageColor
        }
    }

    MouseArea {
        anchors.fill: rootButtonContent
        cursorShape: Qt.PointingHandCursor
        hoverEnabled: root.enabled ? true : false

        onClicked: {
            if (rootButtonClickedFunction && typeof rootButtonClickedFunction === "function") {
                rootButtonClickedFunction()
            } else {
                menu.openTriggered()
            }
        }
    }

    DrawerType2 {
        id: menu

        parent: drawerParent

        anchors.fill: parent
        expandedHeight: drawerParent.height * drawerHeight

        expandedStateContent: Item {
            id: container
            implicitHeight: menu.expandedHeight

            ColumnLayout {
                id: header

                anchors.fill: parent
                anchors.topMargin: 16

                BackButtonType {
                    id: backButton
                    backButtonImage: root.headerBackButtonImage
                    backButtonFunction: function() { menu.closeTriggered() }
                }

                Header2Type {
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.bottomMargin: 16
                    Layout.fillWidth: true

                    headerText: root.headerText
                }

                Loader {
                    id: listViewLoader
                    sourceComponent: root.listView

                    Layout.fillHeight: true
                }
            }
        }
    }
}
