import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PageEnum 1.0
import "./"
import "../Controls"
import "../Controls2"
import "../Config"

PageBase {
    id: root
    page: PageEnum.Test
    logic: ViewConfigLogic

    Rectangle {
        y: 0
        anchors.fill: parent
        color: "#0E0E11"
    }

    FlickableType {
        id: fl
        anchors.top: root.top
        anchors.bottom: root.bottom
        contentHeight: content.height

        ColumnLayout {
            id: content
            enabled: true
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.rightMargin: 15

            BasicButtonType {
                Layout.fillWidth: true
                Layout.topMargin: 10
                text: qsTr("Forget this server")

//                onClicked: {
//                    UiLogic.goToPage(PageEnum.Start)
//                }
            }

            BasicButtonType {
                Layout.fillWidth: true
                Layout.topMargin: 10

                defaultColor: "transparent"
                hoveredColor: Qt.rgba(255, 255, 255, 0.08)
                pressedColor: Qt.rgba(255, 255, 255, 0.12)
                disabledColor: "#878B91"
                textColor: "#D7D8DB"
                borderWidth: 1

                text: qsTr("Forget this server")

//                onClicked: {
//                    UiLogic.goToPage(PageEnum.Start)
//                }
            }

            TextFieldWithHeaderType {
                Layout.fillWidth: true
                Layout.topMargin: 10
                headerText: "Server IP adress [:port]"
            }

            LabelWithButtonType {
                id: ip
                Layout.fillWidth: true
                Layout.topMargin: 10

                text: "IP, логин и пароль от сервера"
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
            LabelWithButtonType {
                Layout.fillWidth: true
                Layout.topMargin: 10

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

            CardType {
                Layout.fillWidth: true
                Layout.topMargin: 10

                headerText: "Высокий"
                bodyText: "Многие иностранные сайты и VPN-провайдеры заблокированы"
                footerText: "футер"
            }

            CardType {
                Layout.fillWidth: true
                Layout.topMargin: 10

                headerText: "Высокий"
                bodyText: "Многие иностранные сайты и VPN-провайдеры заблокированы"
                footerText: "футер"
            }

            CheckBoxType {
//                text: qsTr("Auto-negotiate encryption")
            }
        }
    }
}
