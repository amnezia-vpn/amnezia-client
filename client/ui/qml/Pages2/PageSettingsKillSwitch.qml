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
                        PageController.showNotificationMessage(qsTr("KillSwitch settings cannot be changed during an active connection"))
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
                descriptionText: qsTr("Internet access is blocked if the VPN disconnects unexpectedly")

                onClicked: function() {
                    SettingsController.strictKillSwitchEnabled = false
                }

                Keys.onEnterPressed: this.clicked()
                Keys.onReturnPressed: this.clicked()
            }

            DividerType {}

            VerticalRadioButton {
                id: strictKillSwitch
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                visible: false
                enabled: false
                // enabled: SettingsController.isKillSwitchEnabled && !ConnectionController.isConnected
                checked: SettingsController.strictKillSwitchEnabled

                text: qsTr("Strict KillSwitch")
                descriptionText: qsTr("Internet connection is blocked even when VPN is turned off manually or hasn't started")

                onClicked: function() {
                    var headerText = qsTr("Just a little heads-up")
                    var descriptionText = qsTr("If the VPN disconnects or drops while Strict KillSwitch is enabled, internet access will be blocked. To restore access, reconnect VPN or disable/change the KillSwitch.")
                    var yesButtonText = qsTr("Continue")
                    var noButtonText = qsTr("Cancel")

                    var yesButtonFunction = function() {
                        SettingsController.strictKillSwitchEnabled = true
                    }
                    var noButtonFunction = function() {
                    }

                    showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                }

                Keys.onEnterPressed: this.clicked()
                Keys.onReturnPressed: this.clicked()
            }

            DividerType {
                visible: false
            }
            
            LabelWithButtonType {
                Layout.topMargin: 32
                Layout.fillWidth: true

                enabled: true
                text: qsTr("DNS Exceptions")
                descriptionText: qsTr("DNS servers listed here will remain accessible when KillSwitch is active.")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"

                clickedFunction: function() {
                    PageController.goToPage(PageEnum.PageSettingsKillSwitchExceptions)
                }
            }
        }
    }
} 
