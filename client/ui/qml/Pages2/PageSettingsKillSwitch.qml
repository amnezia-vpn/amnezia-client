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

    FlickableType {
        id: flickable

        anchors.top: parent.top

        contentHeight: content.implicitHeight

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 0

            NavigationBarType {
                id: navigationBar

                Layout.fillWidth: true

                title: qsTr("KillSwitch")
                description: qsTr("Enable to ensure network traffic goes through a secure VPN tunnel, preventing accidental exposure of your IP and DNS queries if the connection drops")

                header.showSwitcher: true
                header.switcher.checked: SettingsController.isKillSwitchEnabled
                header.switcher.enabled: !ConnectionController.isConnected

                Connections {
                    target: navigationBar.header

                    function onSwitcherToggled(checked) {
                        if (!ConnectionController.isConnected) {
                            SettingsController.isKillSwitchEnabled = checked
                        } else {
                            PageController.showNotificationMessage(qsTr("KillSwitch settings cannot be changed during an active connection"))
                            navigationBar.header.switcher.checked = SettingsController.isKillSwitchEnabled
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 16 + PageController.safeAreaBottomMargin

                spacing: 24

                ColumnLayout {
                    Layout.fillWidth: true

                    spacing: 0

                    RadioItemType {
                        id: softKillSwitch

                        Layout.fillWidth: true

                        enabled: SettingsController.isKillSwitchEnabled && !ConnectionController.isConnected
                        checked: !SettingsController.strictKillSwitchEnabled

                        text: qsTr("Soft KillSwitch")
                        subtitle: qsTr("Internet access is blocked if the VPN disconnects unexpectedly")

                        onClicked: function() {
                            SettingsController.strictKillSwitchEnabled = false
                        }
                    }

                    RadioItemType {
                        id: strictKillSwitch

                        Layout.fillWidth: true

                        visible: false
                        enabled: false
                        checked: SettingsController.strictKillSwitchEnabled

                        text: qsTr("Strict KillSwitch")
                        subtitle: qsTr("Internet connection is blocked even when VPN is turned off manually or hasn't started")

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
                    }
                }

                ListItemType {
                    id: dnsExceptionsButton

                    Layout.fillWidth: true

                    showSeparator: false
                    text: qsTr("DNS Exceptions")
                    subtitle: qsTr("DNS servers listed here will remain accessible when KillSwitch is active.")
                    trailingIconSource: "qrc:/images/controls/chevron-right.svg"

                    onClicked: {
                        PageController.goToPage(PageEnum.PageSettingsKillSwitchExceptions)
                    }
                }
            }
        }
    }
}
