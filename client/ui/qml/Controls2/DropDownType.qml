import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "TextTypes"

Item {
    id: root

    property string text
    property string textColor: "#d7d8db"

    property string descriptionText

    property string headerText
    property string headerBackButtonImage

    property var onRootButtonClicked
    property string rootButtonImage: "qrc:/images/controls/chevron-down.svg"
    property string rootButtonImageColor: "#494B50"
    property string rootButtonDefaultColor: "#1C1D21"
    property int rootButtonMaximumWidth

    property string rootButtonBorderColor: "#494B50"
    property int rootButtonBorderWidth: 1

    property Component menuDelegate
    property variant menuModel

    property alias menuVisible: menu.visible

    implicitWidth: rootButtonContent.implicitWidth
    implicitHeight: rootButtonContent.implicitHeight

    Rectangle {
        id: rootButtonBackground
        anchors.fill: rootButtonContent

        radius: 16
        color: rootButtonDefaultColor
        border.color: rootButtonBorderColor
        border.width: rootButtonBorderWidth

        Behavior on border.width {
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
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter

                visible: root.descriptionText !== ""

                color: "#878B91"
                text: root.descriptionText
            }

            ButtonTextType {
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter

                Layout.maximumWidth: rootButtonMaximumWidth ? rootButtonMaximumWidth : implicitWidth

                color: root.textColor
                text: root.text

                wrapMode: Text.NoWrap
                elide: Text.ElideRight
            }
        }

        //todo change to image type
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
        hoverEnabled: true

        onEntered: {
            rootButtonBackground.border.width = rootButtonBorderWidth
        }

        onExited: {
            rootButtonBackground.border.width = 0
        }

        onClicked: {
            if (onRootButtonClicked && typeof onRootButtonClicked === "function") {
                onRootButtonClicked()
            } else {
                menu.visible = true
            }
        }
    }

    Drawer {
        id: menu

        edge: Qt.BottomEdge
        width: parent.width
        height: parent.height * 0.9

        clip: true
        modal: true

        background: Rectangle {
            anchors.fill: parent
            anchors.bottomMargin: -radius
            radius: 16
            color: "#1C1D21"

            border.color: "#494B50"
            border.width: 1
        }

        Overlay.modal: Rectangle {
            color: Qt.rgba(14/255, 14/255, 17/255, 0.8)
        }

        Header2Type {
            id: header

            headerText: root.headerText
            backButtonImage: root.headerBackButtonImage

            width: parent.width

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 16
            anchors.leftMargin: 16
            anchors.rightMargin: 16

            backButtonFunction: function() {
                root.menuVisible = false
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

                ButtonGroup {
                    id: radioButtonGroup
                }

                ListView {
                    id: menuContent
                    width: parent.width
                    height: menuContent.contentItem.height

                    currentIndex: -1

                    clip: true
                    interactive: false

                    model: root.menuModel

                    delegate: Row {
                        Loader {
                            id: loader
                            sourceComponent: root.menuDelegate
                            property QtObject modelData: model
                            property var delegateIndex: index
                        }
                    }
                }
            }
        }
    }
}
