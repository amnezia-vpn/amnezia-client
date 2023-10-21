import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0

import "./"
import "../Controls2"
import "../Config"
import "../Controls2/TextTypes"
import "../Components"

PageType {
    id: root

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20
    }

    FlickableType {
        id: fl
        anchors.top: backButton.bottom
        anchors.bottom: parent.bottom
        contentHeight: content.height

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 16
            anchors.rightMargin: 16

            spacing: 16

            HeaderType {
                Layout.fillWidth: true

                headerText: qsTr("DNS servers")
            }

            ParagraphTextType {
                Layout.fillWidth: true
                text: qsTr("If AmneziaDNS is not used or installed")
            }

            TextFieldWithHeaderType {
                id: primaryDns

                Layout.fillWidth: true
                headerText: qsTr("Primary DNS")

                textFieldText: SettingsController.primaryDns
                textField.validator: RegularExpressionValidator {
                    regularExpression: InstallController.ipAddressRegExp()
                }
            }

            TextFieldWithHeaderType {
                id: secondaryDns

                Layout.fillWidth: true
                headerText: qsTr("Secondary DNS")

                textFieldText: SettingsController.secondaryDns
                textField.validator: RegularExpressionValidator {
                    regularExpression: InstallController.ipAddressRegExp()
                }
            }

            BasicButtonType {
                Layout.fillWidth: true

                defaultColor: "transparent"
                hoveredColor: Qt.rgba(1, 1, 1, 0.08)
                pressedColor: Qt.rgba(1, 1, 1, 0.12)
                disabledColor: "#878B91"
                textColor: "#D7D8DB"
                borderWidth: 1

                text: qsTr("Restore default")

                onClicked: function() {
                    questionDrawer.headerText = qsTr("Restore default DNS settings?")
                    questionDrawer.yesButtonText = qsTr("Continue")
                    questionDrawer.noButtonText = qsTr("Cancel")

                    questionDrawer.yesButtonFunction = function() {
                        questionDrawer.close()
                        SettingsController.primaryDns = "1.1.1.1"
                        primaryDns.textFieldText = SettingsController.primaryDns
                        SettingsController.secondaryDns = "1.0.0.1"
                        secondaryDns.textFieldText = SettingsController.secondaryDns
                        PageController.showNotificationMessage(qsTr("Settings have been reset"))
                    }
                    questionDrawer.noButtonFunction = function() {
                        questionDrawer.close()
                    }
                    questionDrawer.open()
                }
            }

            BasicButtonType {
                Layout.fillWidth: true

                text: qsTr("Save")

                onClicked: function() {
                    if (primaryDns.textFieldText !== SettingsController.primaryDns) {
                        SettingsController.primaryDns = primaryDns.textFieldText
                    }
                    if (secondaryDns.textFieldText !== SettingsController.secondaryDns) {
                        SettingsController.secondaryDns = secondaryDns.textFieldText
                    }
                    PageController.showNotificationMessage(qsTr("Settings saved"))
                }
            }
        }
        QuestionDrawer {
            id: questionDrawer
            parent: root
        }
    }
}
