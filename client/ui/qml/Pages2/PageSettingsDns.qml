import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Style 1.0

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

        onFocusChanged: {
            if (this.activeFocus) {
                listView.positionViewAtBeginning()
            }
        }
    }

    ListViewType {
        id: listView

        anchors.top: backButton.bottom
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.left: parent.left

        property var isServerFromApi: ServersModel.isServerFromApi(ServersModel.defaultIndex)

        enabled: !isServerFromApi

        Component.onCompleted: {
            if (isServerFromApi) {
                PageController.showNotificationMessage(qsTr("Default server does not support custom DNS"))
            }
        }

        header: ColumnLayout {
            width: listView.width
            spacing: 16

            BaseHeaderType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("DNS servers")
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                
                text: qsTr("If AmneziaDNS is not used or installed")
            }

            TextFieldWithHeaderType {
                id: primaryDns

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("Primary DNS")

                textField.text: SettingsController.primaryDns
                textField.validator: RegularExpressionValidator {
                    regularExpression: InstallController.ipAddressRegExp()
                }
            }

            TextFieldWithHeaderType {
                id: secondaryDns

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("Secondary DNS")

                textField.text: SettingsController.secondaryDns
                textField.validator: RegularExpressionValidator {
                    regularExpression: InstallController.ipAddressRegExp()
                }
            }
        }

        model: 1 // fake model to force the ListView to be created without a model
        spacing: 16

        delegate: ColumnLayout {
            width: listView.width

            BasicButtonType {
                id: restoreDefaultButton

                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                defaultColor: AmneziaStyle.color.transparent
                hoveredColor: AmneziaStyle.color.translucentWhite
                pressedColor: AmneziaStyle.color.sheerWhite
                disabledColor: AmneziaStyle.color.mutedGray
                textColor: AmneziaStyle.color.paleGray
                borderWidth: 1

                text: qsTr("Restore default")

                clickedFunc: function() {
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

                    showQuestionDrawer(headerText, "", yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                }
            }
        }

        footer: ColumnLayout {
            width: listView.width

            BasicButtonType {
                id: saveButton

                Layout.fillWidth: true
                Layout.margins: 16

                text: qsTr("Save")

                clickedFunc: function() {
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
}
