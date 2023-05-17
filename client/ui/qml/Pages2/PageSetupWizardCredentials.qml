import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0

import "./"
import "../Controls2"
import "../Config"

Item {
    id: root

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

            HeaderType {
                Layout.fillWidth: true
                Layout.topMargin: 20

                backButtonImage: "qrc:/images/controls/arrow-left.svg"

                headerText: "Подключение к серверу"
            }

            TextFieldWithHeaderType {
                id: hostname

                Layout.fillWidth: true
                headerText: "Server IP adress [:port]"
            }

            TextFieldWithHeaderType {
                id: username

                Layout.fillWidth: true
                headerText: "Login to connect via SSH"
            }

            TextFieldWithHeaderType {
                id: secretData

                Layout.fillWidth: true
                headerText: "Password / Private key"
                textField.echoMode: TextInput.Password
            }

            BasicButtonType {
                Layout.fillWidth: true
                Layout.topMargin: 24

                text: qsTr("Настроить сервер простым образом")

                onClicked: function() {
                    PageController.goToPage(PageEnum.PageSetupWizardEasy)
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
                    InstallController.setShouldCreateServer(true)
                    InstallController.setCurrentlyInstalledServerCredentials(hostname.textField.text, username.textField.text, secretData.textField.text)

                    PageController.goToPage(PageEnum.PageSetupWizardProtocols)
                }
            }
        }
    }
}
