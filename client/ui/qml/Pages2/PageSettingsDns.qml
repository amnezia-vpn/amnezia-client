import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Config"
import "../Components"

PageType {
    id: root

    property bool isServerFromApi: ServersUiController.isDefaultServerFromApi

    Component.onCompleted: {
        if (root.isServerFromApi) {
            PageController.showNotificationMessage(qsTr("Default server does not support custom DNS"))
        }
    }

    FlickableType {
        id: flickable

        anchors.top: parent.top
        anchors.bottom: buttons.top

        contentHeight: content.implicitHeight

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 0

            NavigationBarType {
                Layout.fillWidth: true

                title: qsTr("DNS servers")
                description: qsTr("If AmneziaDNS is not used or installed")
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 24

                spacing: 12

                enabled: !root.isServerFromApi

                TextFieldType {
                    id: primaryDns

                    Layout.fillWidth: true

                    headerText: qsTr("Primary DNS")

                    textField.text: SettingsController.primaryDns
                    textField.validator: RegularExpressionValidator {
                        regularExpression: InstallController.ipAddressRegExp()
                    }
                }

                TextFieldType {
                    id: secondaryDns

                    Layout.fillWidth: true

                    headerText: qsTr("Secondary DNS")

                    textField.text: SettingsController.secondaryDns
                    textField.validator: RegularExpressionValidator {
                        regularExpression: InstallController.ipAddressRegExp()
                    }
                }
            }
        }
    }

    ColumnLayout {
        id: buttons

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        anchors.bottomMargin: 8 + PageController.safeAreaBottomMargin

        spacing: 12

        enabled: !root.isServerFromApi

        ButtonType {
            id: restoreDefaultButton

            Layout.fillWidth: true

            variant: ButtonType.Secondary
            text: qsTr("Restore default")

            onClicked: {
                var headerText = qsTr("Restore default DNS settings?")
                var yesButtonText = qsTr("Continue")
                var noButtonText = qsTr("Cancel")

                var yesButtonFunction = function() {
                    SettingsController.primaryDns = "1.1.1.1"
                    primaryDns.textField.text = SettingsController.primaryDns
                    SettingsController.secondaryDns = "1.0.0.1"
                    secondaryDns.textField.text = SettingsController.secondaryDns
                    PageController.showNotificationMessage(qsTr("Settings have been reset"))
                }
                var noButtonFunction = function() {
                }

                Dialogs.askQuestion(headerText, "", yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
            }
        }

        ButtonType {
            id: saveButton

            Layout.fillWidth: true

            text: qsTr("Save")

            onClicked: {
                if (primaryDns.textField.text === "") {
                    primaryDns.errorText = qsTr("Primary DNS cannot be empty")
                    return
                }
                primaryDns.errorText = ""
                secondaryDns.errorText = ""

                if (primaryDns.textField.text !== SettingsController.primaryDns) {
                    SettingsController.primaryDns = primaryDns.textField.text
                }
                if (secondaryDns.textField.text !== SettingsController.secondaryDns) {
                    SettingsController.secondaryDns = secondaryDns.textField.text
                }
                PageController.showNotificationMessage(qsTr("Settings saved"))
            }
        }
    }
}
