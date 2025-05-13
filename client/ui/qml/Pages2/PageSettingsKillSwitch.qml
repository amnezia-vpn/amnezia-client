import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Config"

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

            HeaderTypeWithSwitcher {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("KillSwitch")
                descriptionText: qsTr("Enable to ensure network traffic goes through a secure VPN tunnel, preventing accidental exposure of your IP and DNS queries if the connection drops")

                showSwitcher: true
                switcher {
                    checked: SettingsController.isKillSwitchEnabled
                    enabled: !ConnectionController.isConnected
                }
                switcherFunction: function(checked) {
                    if (!ConnectionController.isConnected) {
                        SettingsController.isKillSwitchEnabled = checked
                    } else {
                        PageController.showNotificationMessage(qsTr("Cannot change killSwitch settings during active connection"))
                        switcher.checked = SettingsController.isKillSwitchEnabled
                    }
                }
            }

            VerticalRadioButton {
                id: softKillSwitch
                Layout.fillWidth: true
                Layout.topMargin: 32
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                enabled: SettingsController.isKillSwitchEnabled && !ConnectionController.isConnected
                checked: !SettingsController.strictKillSwitchEnabled

                text: qsTr("Soft KillSwitch")
                descriptionText: qsTr("Internet connection is blocked if VPN connection drops accidentally")

                onClicked: function() {
                    SettingsController.strictKillSwitchEnabled = false
                }
            }

            DividerType {}

            VerticalRadioButton {
                id: strictKillSwitch
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                enabled: SettingsController.isKillSwitchEnabled && !ConnectionController.isConnected
                checked: SettingsController.strictKillSwitchEnabled

                text: qsTr("Strict KillSwitch")
                descriptionText: qsTr("Internet connection is blocked even if VPN was turned off manually or not started")

                onClicked: function() {
                    var headerText = qsTr("Just a little heads-up")
                    var descriptionText = qsTr("If you disconnect from VPN or the VPN connection drops while the Strict Kill Switch is turned on, your internet access will be disabled. To restore it, connect to VPN, change the Kill Switch mode or turn the Kill Switch off.")
                    var yesButtonText = qsTr("Continue")
                    var noButtonText = qsTr("Cancel")

                    var yesButtonFunction = function() {
                        SettingsController.strictKillSwitchEnabled = true
                    }
                    var noButtonFunction = function() {
                    }

                    showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                }
            }

            DividerType {}
            
            LabelWithButtonType {
                Layout.topMargin: 32
                Layout.fillWidth: true

                enabled: true
                text: qsTr("DNS Exceptions")
                descriptionText: qsTr("DNS servers from the list will remain accessible when Kill Switch is triggered")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"

                clickedFunction: function() {
                    PageController.goToPage(PageEnum.PageSettingsKillSwitchExceptions)
                }
            }
        }
    }
} 
