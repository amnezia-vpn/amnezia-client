import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "TextTypes"
import "../Config"

Item {
    id: root

    property string text
    property string textColor: "#d7d8db"
    property string textDisabledColor: "#878B91"
    property int textMaximumLineCount: 2
    property int textElide: Qt.ElideRight

    property string descriptionText
    property string descriptionTextColor: "#878B91"
    property string descriptionTextDisabledColor: "#494B50"

    property string headerText
    property string headerBackButtonImage

    property var rootButtonClickedFunction
    property string rootButtonImage: "qrc:/images/controls/chevron-down.svg"
    property string rootButtonImageColor: "#D7D8DB"
    property string rootButtonBackgroundColor: "#1C1D21"
    property string rootButtonBackgroundHoveredColor: "#1C1D21"
    property string rootButtonBackgroundPressedColor: "#1C1D21"

    property string borderFocusedColor: "#D7D8DB"
    property int borderFocusedWidth: 1

    property string rootButtonHoveredBorderColor: "#494B50"
    property string rootButtonDefaultBorderColor: "#2C2D30"
    property string rootButtonPressedBorderColor: "#D7D8DB"

    property int rootButtonTextLeftMargins: 16
    property int rootButtonTextTopMargin: 16
    property int rootButtonTextBottomMargin: 16

    property real drawerHeight: 0.9
    property Item drawerParent
    property Component listView

    signal open
    signal close

    function popupClosedFunc() {
        if (!GC.isMobile()) {
            this.forceActiveFocus()
        }
    }

    property var parentFlickable
    onFocusChanged: {
        if (root.activeFocus) {
            if (root.parentFlickable) {
                root.parentFlickable.ensureVisible(root)
            }
        }
    }

    implicitWidth: rootButtonContent.implicitWidth
    implicitHeight: rootButtonContent.implicitHeight

    onOpen: {
        menu.open()
    }

    onClose: {
        menu.close()
    }

    Rectangle {
        id: focusBorder

        color: "transparent"
        border.color: root.activeFocus ? root.borderFocusedColor : "transparent"
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
                    return "transparent"
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
                menu.open()
            }
        }
    }

    DrawerType2 {
        id: menu

        parent: drawerParent

        anchors.fill: parent
        expandedHeight: drawerParent.height * drawerHeight

        onClosed: {
            root.popupClosedFunc()
        }

        expandedContent: Item {
            id: container
            implicitHeight: menu.expandedHeight

            Connections {
                target: menu
                enabled: !GC.isMobile()
                function onOpened() {
                    listViewLoader.item.forceActiveFocus()
                }
            }

            ColumnLayout {
                id: header

                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.topMargin: 16

                BackButtonType {
                    backButtonImage: root.headerBackButtonImage
                    backButtonFunction: function() {
                        menu.close()
                    }
                }
            }

            FlickableType {
                id: flickable
                anchors.top: header.bottom
                anchors.topMargin: 16
                contentHeight: col.implicitHeight

                Column {
                    id: col
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right

                    spacing: 16

                    Header2Type {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.leftMargin: 16
                        anchors.rightMargin: 16

                        headerText: root.headerText

                        width: parent.width
                    }

                    Loader {
                        id: listViewLoader
                        sourceComponent: root.listView

                        onLoaded: {
                            listViewLoader.item.parentFlickable = flickable
                        }
                    }
                }
            }
        }
    }

    Keys.onEnterPressed: {
        if (menu.isClosed) {
            menu.open()
        }
    }

    Keys.onReturnPressed: {
        if (menu.isClosed) {
            menu.open()
        }
    }
}
