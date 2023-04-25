import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property string text
    property string descriptionText

    property var onClickedFunc
    property string buttonImage: "qrc:/images/controls/chevron-down.svg"

    property string defaultColor: "#1C1D21"

    property string borderColor: "#494B50"

    property alias menuModel: menuContent.model

    width: buttonContent.implicitWidth
    height: buttonContent.implicitHeight

    Rectangle {
        id: buttonBackground
        anchors.fill: buttonContent

        radius: 16
        color: defaultColor
        border.color: borderColor

        Behavior on border.width {
            PropertyAnimation { duration: 200 }
        }
    }

    RowLayout {
        id: buttonContent
        anchors.fill: parent
        anchors.rightMargin: 16
        anchors.leftMargin: 16

        ColumnLayout {
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.topMargin: 16
            Layout.bottomMargin: 16

            Text {
                visible: root.descriptionText !== ""

                font.family: "PT Root UI"
                font.styleName: "normal"
                font.pixelSize: 13
                font.letterSpacing: 0.02
                color: "#878B91"
                text: root.descriptionText
                wrapMode: Text.WordWrap

                Layout.fillWidth: true
                height: 16   

                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
            }

            Text {
                font.family: "PT Root UI"
                font.styleName: "normal"
                font.pixelSize: 16
                color: "#d7d8db"
                text: root.text

                Layout.fillWidth: true
                height: 24

                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
            }
        }

        ImageButtonType {
            id: button

            Layout.rightMargin: 16

            hoverEnabled: false
            image: buttonImage
            onClicked: {
                if (onClickedFunc && typeof onClickedFunc === "function") {
                    onClickedFunc()
                }
            }

            Layout.alignment: Qt.AlignRight
        }
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        hoverEnabled: true

        onEntered: {
            buttonBackground.border.width = 1
        }

        onExited: {
            buttonBackground.border.width = 0
        }

        onClicked: {
            menu.visible = true
        }
    }

    Drawer {
        id: menu

        edge: Qt.BottomEdge
        width: parent.width
        height: parent.height * 0.8

        clip: true
        modal: true

        background: Rectangle {
            anchors.fill: parent
            anchors.bottomMargin: -radius
            radius: 16
            color: "#1C1D21"
        }

        Overlay.modal: Rectangle {
            color: Qt.rgba(14/255, 14/255, 17/255, 0.8)
        }

        Column {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 16

            spacing: 16

            Header2TextType {
                width: parent.width

                text: "Данные для подключения"
                wrapMode: Text.WordWrap

                leftPadding: 16
                rightPadding: 16
            }

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

                delegate: Item {
                    implicitWidth: menuContent.width
                    implicitHeight: radioButton.implicitHeight

                    RadioButton {
                        id: radioButton

                        implicitWidth: parent.width
                        implicitHeight: radioButtonContent.implicitHeight

                        hoverEnabled: true

                        ButtonGroup.group: radioButtonGroup

                        indicator: Rectangle {
                            anchors.fill: parent
                            color: radioButton.hovered ? "#2C2D30" : "#1C1D21"
                        }

                        RowLayout {
                            id: radioButtonContent
                            anchors.fill: parent

                            anchors.rightMargin: 16
                            anchors.leftMargin: 16

                            z: 1

                            Text {
                                id: text

                                text: modelData
                                color: "#D7D8DB"
                                font.pixelSize: 16
                                font.weight: 400
                                font.family: "PT Root UI VF"

                                height: 24

                                Layout.fillWidth: true
                                Layout.topMargin: 20
                                Layout.bottomMargin: 20
                            }

                            Image {
                                source: "qrc:/images/controls/check.svg"
                                visible: radioButton.checked
                                width: 24
                                height: 24

                                Layout.rightMargin: 8
                            }
                        }
                    }
                }
            }
        }
    }
}
