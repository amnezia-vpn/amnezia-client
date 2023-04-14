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
    page: PageEnum.WizardCredentials

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
            anchors.rightMargin: 16
            anchors.leftMargin: 16

            spacing: 16

            HeaderTextType {
                Layout.fillWidth: true
                Layout.topMargin: 20

                buttonImage: "qrc:/images/controls/arrow-left.svg"

                headerText: "Подключение к серверу"
            }

            TextFieldWithHeaderType {
                Layout.fillWidth: true
                headerText: "Server IP adress [:port]"
            }

            TextFieldWithHeaderType {
                Layout.fillWidth: true
                headerText: "Login to connect via SSH"
            }

            TextFieldWithHeaderType {
                Layout.fillWidth: true
                headerText: "Password / Private key"
            }

            BasicButtonType {
                Layout.fillWidth: true
                Layout.topMargin: 24

                text: qsTr("Настроить сервер простым образом")

                onClicked: function() {
                    UiLogic.goToPage(PageEnum.WizardEasySetup)
                }
            }

            BasicButtonType {
                Layout.fillWidth: true
                Layout.topMargin: -8

                defaultColor: "transparent"
                hoveredColor: Qt.rgba(1, 1, 1, 0.08)
                pressedColor: Qt.rgba(1, 1, 1, 0.12)
                disabledColor: "#878B91"
                textColor: "#D7D8DB"
                borderWidth: 1

                text: qsTr("Выбрать протокол для установки")

                onClicked: function() {
                    UiLogic.goToPage(PageEnum.WizardProtocols)
                }
            }
        }
    }
}
