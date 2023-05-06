import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0

import "./"
import "../Pages"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"

PageBase {
    id: root
    page: PageEnum.PageHome

    property string defaultColor: "#1C1D21"

    property string borderColor: "#2C2D30"

    property string currentServerName: menuContent.currentItem.delegateData.desc
    property string currentServerDescription: menuContent.currentItem.delegateData.address

    Rectangle {
        id: buttonBackground
        anchors.fill: buttonContent
        anchors.bottomMargin: -radius

        radius: 16
        color: defaultColor
        border.color: borderColor
        border.width: 1

        Rectangle {
            width: parent.width
            height: 1
            y: parent.height - height - parent.radius

            color: borderColor
        }
    }

    ColumnLayout {
        id: buttonContent
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.bottom: parent.bottom

        RowLayout {
            Layout.topMargin: 24
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

            Header1TextType {
                text: currentServerName
            }

            Image {
                Layout.preferredWidth: 18
                Layout.preferredHeight: 18

                source: "qrc:/images/controls/chevron-down.svg"
            }
        }

        LabelTextType {
            Layout.bottomMargin: 44
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

            text: currentServerDescription
        }
    }

    MouseArea {
        anchors.fill: buttonBackground
        cursorShape: Qt.PointingHandCursor
        hoverEnabled: true

        onClicked: {
            menu.visible = true
        }
    }

    Drawer {
        id: menu

        edge: Qt.BottomEdge
        width: parent.width
        height: parent.height * 0.90

        clip: true
        modal: true

        background: Rectangle {
            anchors.fill: parent
            anchors.bottomMargin: -radius
            radius: 16

            color: "#1C1D21"
            border.color: borderColor
            border.width: 1
        }

        Overlay.modal: Rectangle {
            color: Qt.rgba(14/255, 14/255, 17/255, 0.8)
        }

        ColumnLayout {
            id: menuHeader
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.left: parent.left

            Header1TextType {
                Layout.topMargin: 24
                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

                text: currentServerName
            }

            LabelTextType {
                Layout.bottomMargin: 24
                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

                text: currentServerDescription
            }

            RowLayout {

                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                spacing: 8

                DropDownType {
                    implicitHeight: 40

                    borderWidth: 0
                    buttonImageColor: "#0E0E11"

                    defaultColor: "#D7D8DB"

                    textColor: "#0E0E11"
                    text: "testtesttest"
                }

                BasicButtonType {
                    implicitHeight: 40

                    text: "Amnezia DNS"
                }
            }

            Header2Type {
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: "Серверы"
            }
        }


//        Header2TextType {
//            id: menuHeader
//            width: parent.width

//            text: "Данные для подключения"
//            wrapMode: Text.WordWrap

//            anchors.top: parent.top
//            anchors.left: parent.left
//            anchors.right: parent.right
//            anchors.topMargin: 16
//            anchors.leftMargin: 16
//            anchors.rightMargin: 16
//        }

        FlickableType {
            anchors.top: menuHeader.bottom
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

                    model: ServersModel
                    currentIndex: 0

                    clip: true
                    interactive: false

                    delegate: Item {
                        id: menuContentDelegate

                        property variant delegateData: model

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

                                    text: desc
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
}
