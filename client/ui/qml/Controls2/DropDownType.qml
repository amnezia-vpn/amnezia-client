import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "TextTypes"

Item {
    id: root

    property string text
    property string textColor: "#d7d8db"
    property string textDisabledColor: "#878B91"

    property string descriptionText
    property string descriptionTextColor: "#878B91"
    property string descriptionTextDisabledColor: "#494B50"

    property string headerText
    property string headerBackButtonImage

    property var rootButtonClickedFunction
    property string rootButtonImage: "qrc:/images/controls/chevron-down.svg"
    property string rootButtonImageColor: "#D7D8DB"
    property string rootButtonBackgroundColor: "#1C1D21"

    property string rootButtonHoveredBorderColor: "#494B50"
    property string rootButtonDefaultBorderColor: "transparent"
    property string rootButtonPressedBorderColor: "#D7D8DB"

    property real drawerHeight: 0.9
    property Component listView

    property alias menuVisible: menu.visible

    implicitWidth: rootButtonContent.implicitWidth
    implicitHeight: rootButtonContent.implicitHeight

    onMenuVisibleChanged: {
        if (menuVisible) {
            rootButtonBackground.border.color = rootButtonPressedBorderColor
        } else {
            rootButtonBackground.border.color = rootButtonDefaultBorderColor
        }
    }

    onEnabledChanged: {
        if (enabled) {
            rootButtonBackground.color = rootButtonBackgroundColor
            rootButtonBackground.border.color = rootButtonDefaultBorderColor
        } else {
            rootButtonBackground.color = "transparent"
            rootButtonBackground.border.color = rootButtonHoveredBorderColor
        }
    }

    Rectangle {
        id: rootButtonBackground
        anchors.fill: rootButtonContent

        radius: 16
        color: root.enabled ? rootButtonBackgroundColor : "transparent"
        border.color: root.enabled ? rootButtonDefaultBorderColor : rootButtonHoveredBorderColor
        border.width: 1

        Behavior on border.color {
            PropertyAnimation { duration: 200 }
        }
    }

    RowLayout {
        id: rootButtonContent
        anchors.fill: parent

        spacing: 0

        ColumnLayout {
            Layout.leftMargin: 16

            LabelTextType {
                Layout.fillWidth: true

                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter

                visible: root.descriptionText !== ""

                color: root.enabled ? root.descriptionTextColor : root.descriptionTextDisabledColor
                text: root.descriptionText
            }

            ButtonTextType {
                Layout.fillWidth: true

                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter

                color: root.enabled ? root.textColor : root.textDisabledColor
                text: root.text

                wrapMode: Text.NoWrap
                elide: Text.ElideRight
            }
        }

        ImageButtonType {
            Layout.leftMargin: 4
            Layout.rightMargin: 16

            hoverEnabled: false
            image: rootButtonImage
            imageColor: rootButtonImageColor
        }
    }

    MouseArea {
        anchors.fill: rootButtonContent
        cursorShape: Qt.PointingHandCursor
        hoverEnabled: root.enabled ? true : false

        onEntered: {
            if (menu.visible === false) {
                rootButtonBackground.border.color = rootButtonHoveredBorderColor
            }
        }

        onExited: {
            if (menu.visible === false) {
                rootButtonBackground.border.color = rootButtonDefaultBorderColor
            }
        }

        onClicked: {
            if (rootButtonClickedFunction && typeof rootButtonClickedFunction === "function") {
                rootButtonClickedFunction()
            } else {
                menu.visible = true
            }
        }
    }

    DrawerType {
        id: menu

        width: parent.width
        height: parent.height * drawerHeight

        ColumnLayout {
            id: header

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 16

            BackButtonType {
                backButtonImage: root.headerBackButtonImage
                backButtonFunction: function() {
                    root.menuVisible = false
                }
            }
        }

        FlickableType {
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
                }
            }
        }
    }
}
