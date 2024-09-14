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
    }

    FlickableType {
        id: fl
        anchors.top: backButton.bottom
        anchors.bottom: parent.bottom
        contentHeight: content.height

        property var isServerFromApi: ServersModel.isServerFromApi(ServersModel.defaultIndex)

        enabled: !isServerFromApi

        Component.onCompleted: {
            if (isServerFromApi) {
                PageController.showNotificationMessage(qsTr("Default server does not support custom DNS"))
            }
        }

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
                id: restoreDefaultButton
                Layout.fillWidth: true

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
                        primaryDns.textFieldText = SettingsController.primaryDns
                        SettingsController.secondaryDns = "1.0.0.1"
                        secondaryDns.textFieldText = SettingsController.secondaryDns
                        PageController.showNotificationMessage(qsTr("Settings have been reset"))

                        if (!GC.isMobile()) {
                            // defaultActiveFocusItem.forceActiveFocus()
                        }
                    }
                    var noButtonFunction = function() {
                        if (!GC.isMobile()) {
                            // defaultActiveFocusItem.forceActiveFocus()
                        }
                    }

                    showQuestionDrawer(headerText, "", yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                }
            }

            BasicButtonType {
                id: saveButton

                Layout.fillWidth: true

                text: qsTr("Save")

                clickedFunc: function() {
                    if (primaryDns.textFieldText !== SettingsController.primaryDns) {
                        SettingsController.primaryDns = primaryDns.textFieldText
                    }
                    if (secondaryDns.textFieldText !== SettingsController.secondaryDns) {
                        SettingsController.secondaryDns = secondaryDns.textFieldText
                    }
                    PageController.showNotificationMessage(qsTr("Settings saved"))
                }

                Keys.onTabPressed: lastItemTabClicked(focusItem)
            }
        }
    }

}
