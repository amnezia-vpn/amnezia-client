import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0

import "./"
import "../Pages"
import "../Controls2"
import "../Config"

PageBase {
    id: root
    page: PageEnum.Start

    FlickableType {
        id: fl
        anchors.top: root.top
        anchors.bottom: root.bottom
        contentHeight: content.height

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            Image {
                id: image
                source: "qrc:/images/amneziaBigLogo.png"

                Layout.alignment: Qt.AlignCenter
                Layout.topMargin: 80
                Layout.leftMargin: 8
                Layout.rightMargin: 8
                Layout.preferredWidth: 344
                Layout.preferredHeight: 279
            }

            BodyTextType {
                Layout.fillWidth: true
                Layout.topMargin: 50
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: "Бесплатный сервис для создания личного VPN на вашем сервере. Помогаем получать доступ к заблокированному контенту, не раскрывая конфиденциальность даже провайдерам VPN."
            }

            BasicButtonType {
                Layout.fillWidth: true
                Layout.topMargin: 32
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("У меня есть данные для подключения")

                onClicked: {
                    drawer.visible = true
                }
            }

            BasicButtonType {
                Layout.fillWidth: true
                Layout.topMargin: 8
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                defaultColor: "transparent"
                hoveredColor: Qt.rgba(255, 255, 255, 0.08)
                pressedColor: Qt.rgba(255, 255, 255, 0.12)
                disabledColor: "#878B91"
                textColor: "#D7D8DB"
                borderWidth: 1

                text: qsTr("У меня ничего нет")

//                onClicked: {
//                    UiLogic.goToPage(PageEnum.Start)
//                }
            }
        }

        Drawer {
            id: drawer

            y: 0
            x: 0
            edge: Qt.BottomEdge
            width: parent.width
            height: parent.height * 0.4375

            clip: true

            background: Rectangle {
                anchors.fill: parent
                anchors.bottomMargin: -radius
                radius: 16
                color: "#1C1D21"
            }

            modal: true
            //interactive: activeFocus

//            onAboutToHide: {
//                pageLoader.focus = true
//            }
//            onAboutToShow: {
//                tfSshLog.focus = true
//            }

            ColumnLayout {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right

                anchors.rightMargin: 16
                anchors.leftMargin: 16

                Header2TextType {
                    Layout.fillWidth: true
                    Layout.topMargin: 24
                    Layout.alignment: Qt.AlignHCenter

                    text: "Данные для подключения"
                }

                LabelWithButtonType {
                    id: ip
                    Layout.fillWidth: true
                    Layout.topMargin: 32

                    text: "IP, логин и пароль от сервера"
                    buttonImage: "qrc:/images/controls/chevron-right.svg"

                    onClickedFunc: function() {
                        UiLogic.goToPage(PageEnum.Credentials)
                    }
                }
                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: "#2C2D30"
                }
                LabelWithButtonType {
                    Layout.fillWidth: true

                    text: "QR-код, ключ или файл настроек"
                    buttonImage: "qrc:/images/controls/chevron-right.svg"

    //                onClickedFunc: function() {
    //                    UiLogic.goToPage(PageEnum.Start)
    //                }
                }
                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: "#2C2D30"
                }
            }
        }
    }
}
