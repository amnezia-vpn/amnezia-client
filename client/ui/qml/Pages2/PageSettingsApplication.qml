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

    Component.onCompleted: SettingsController.refreshVpnDeepLinkStatus()
    onVisibleChanged: {
        if (visible) {
            SettingsController.refreshVpnDeepLinkStatus()
        }
    }

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
                Layout.fillWidth: true

                title: qsTr("Application")
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 16 + PageController.safeAreaBottomMargin

                spacing: 0

                ListItemType {
                    id: switcherAllowScreenshotsRow

                    Layout.fillWidth: true

                    visible: GC.isMobile()

                    text: qsTr("Allow application screenshots")

                    onClicked: {
                        switcherAllowScreenshots.toggle()
                        switcherAllowScreenshots.toggled()
                    }

                    SwitchType {
                        id: switcherAllowScreenshots

                        focusPolicy: Qt.NoFocus

                        checked: SettingsController.isScreenshotsEnabled()
                        onToggled: function() {
                            if (checked !== SettingsController.isScreenshotsEnabled()) {
                                SettingsController.toggleScreenshotsEnabled(checked)
                            }
                        }
                    }
                }

                ListItemType {
                    id: labelWithButtonNotification

                    Layout.fillWidth: true

                    visible: Qt.platform.os === "android" && !SettingsController.isNotificationPermissionGranted

                    text: qsTr("Enable notifications")
                    subtitle: qsTr("Enable notifications to show the VPN state in the status bar")
                    trailingIconSource: "qrc:/images/controls/chevron-right.svg"

                    onClicked: {
                        SettingsController.requestNotificationPermission()
                    }
                }

                ListItemType {
                    id: switcherAutoStartRow

                    Layout.fillWidth: true

                    visible: !GC.isMobile()

                    text: qsTr("Auto start")
                    subtitle: qsTr("Launch the application every time the device is starts")

                    onClicked: {
                        switcherAutoStart.toggle()
                        switcherAutoStart.toggled()
                    }

                    SwitchType {
                        id: switcherAutoStart

                        focusPolicy: Qt.NoFocus

                        checked: SettingsController.autoStartEnabled
                        onToggled: function() {
                            if (checked !== SettingsController.autoStartEnabled) {
                                SettingsController.toggleAutoStart(checked)
                            }
                        }
                    }
                }

                ListItemType {
                    id: switcherAutoConnectRow

                    Layout.fillWidth: true

                    visible: !GC.isMobile()

                    text: qsTr("Auto connect")
                    subtitle: qsTr("Connect to VPN on app start")

                    onClicked: {
                        switcherAutoConnect.toggle()
                        switcherAutoConnect.toggled()
                    }

                    SwitchType {
                        id: switcherAutoConnect

                        focusPolicy: Qt.NoFocus

                        checked: SettingsController.isAutoConnectEnabled()
                        onToggled: function() {
                            if (checked !== SettingsController.isAutoConnectEnabled()) {
                                SettingsController.toggleAutoConnect(checked)
                            }
                        }
                    }
                }

                ListItemType {
                    id: switcherStartMinimizedRow

                    Layout.fillWidth: true

                    visible: !GC.isMobile()
                    enabled: SettingsController.autoStartEnabled

                    text: qsTr("Start minimized")
                    subtitle: qsTr("Launch application minimized (works with autostart option turned on)")

                    onClicked: {
                        switcherStartMinimized.toggle()
                        switcherStartMinimized.toggled()
                    }

                    SwitchType {
                        id: switcherStartMinimized

                        focusPolicy: Qt.NoFocus

                        checked: SettingsController.autoStartEnabled && SettingsController.startMinimized
                        onToggled: function() {
                            if (checked !== SettingsController.startMinimized) {
                                SettingsController.toggleStartMinimized(checked)
                            }
                        }
                    }
                }

                ListItemType {
                    id: switcherNewsNotificationEnabledRow

                    Layout.fillWidth: true

                    visible: ServersUiController.hasServersFromGatewayApi

                    text: qsTr("News Notification")
                    subtitle: qsTr("Show a notification icon for unread news")

                    onClicked: {
                        switcherNewsNotificationEnabled.toggle()
                        switcherNewsNotificationEnabled.toggled()
                    }

                    SwitchType {
                        id: switcherNewsNotificationEnabled

                        focusPolicy: Qt.NoFocus

                        checked: SettingsController.isNewsNotificationsEnabled()
                        onToggled: function() {
                            if (checked !== SettingsController.isNewsNotificationsEnabled()) {
                                SettingsController.toggleNewsNotificationsEnabled(checked)
                            }
                        }
                    }
                }

                ListItemType {
                    id: switcherAutoUpdateCheckRow

                    Layout.fillWidth: true

                    text: qsTr("Check for updates automatically")
                    subtitle: qsTr("Check for a new app version at startup")

                    onClicked: {
                        switcherAutoUpdateCheck.toggle()
                        switcherAutoUpdateCheck.toggled()
                    }

                    SwitchType {
                        id: switcherAutoUpdateCheck

                        focusPolicy: Qt.NoFocus

                        checked: SettingsController.isAutoUpdateCheckEnabled()
                        onToggled: function() {
                            if (checked !== SettingsController.isAutoUpdateCheckEnabled()) {
                                SettingsController.toggleAutoUpdateCheckEnabled(checked)
                            }
                        }
                    }
                }

                ListItemType {
                    id: labelWithButtonLanguage

                    Layout.fillWidth: true

                    text: qsTr("Language")
                    subtitle: LanguageUiController.currentLanguageName
                    trailingIconSource: "qrc:/images/controls/chevron-right.svg"

                    onClicked: {
                        selectLanguageDrawer.open()
                    }
                }

                ListItemType {
                    id: labelWithButtonLogging

                    Layout.fillWidth: true

                    text: qsTr("Logging")
                    subtitle: SettingsController.isLoggingEnabled ? qsTr("Enabled") : qsTr("Disabled")
                    trailingIconSource: "qrc:/images/controls/chevron-right.svg"

                    onClicked: {
                        PageController.goToPage(PageEnum.PageSettingsLogging)
                    }
                }

                ListItemType {
                    id: switcherVpnDeepLinkRow

                    Layout.fillWidth: true

                    visible: !GC.isMobile()

                    text: qsTr("Open vpn:// links")
                    subtitle: qsTr("Import a key when clicking a vpn:// link in the browser")

                    onClicked: {
                        switcherVpnDeepLink.toggle()
                        switcherVpnDeepLink.toggled()
                    }

                    SwitchType {
                        id: switcherVpnDeepLink

                        focusPolicy: Qt.NoFocus

                        checked: SettingsController.vpnDeepLinkEnabled
                        onToggled: function() {
                            if (checked !== SettingsController.vpnDeepLinkEnabled) {
                                SettingsController.toggleVpnDeepLink(checked)
                            }
                        }
                    }
                }

                ListItemType {
                    id: vpnDeepLinkOverrideRow

                    Layout.fillWidth: true

                    visible: !GC.isMobile() && SettingsController.vpnDeepLinkEnabled && SettingsController.vpnDeepLinkOverridden

                    text: qsTr("Override alias settings for vpn:// links")
                    subtitle: qsTr("vpn:// links are currently opened by another application")
                    trailingIconSource: "qrc:/images/controls/chevron-right.svg"

                    onClicked: {
                        SettingsController.makeVpnDeepLinkDefault()
                    }
                }

                ListItemType {
                    id: labelWithButtonReset

                    Layout.fillWidth: true

                    text: qsTr("Reset settings and remove all data from the application")
                    trailingIconSource: "qrc:/images/controls/chevron-right.svg"
                    destructive: true

                    onClicked: {
                        var headerText = qsTr("Reset settings and remove all data from the application?")
                        var descriptionText = qsTr("All settings will be reset to default. All installed AmneziaVPN services will still remain on the server.")
                        var yesButtonText = qsTr("Continue")
                        var noButtonText = qsTr("Cancel")

                        var yesButtonFunction = function() {
                            if (ServersUiController.isDefaultServerCurrentlyProcessed() && ConnectionController.isConnected) {
                                PageController.showNotificationMessage(qsTr("Cannot reset settings during active connection"))
                            } else {
                                SettingsController.clearSettings()
                                PageController.goToPageHome()
                            }
                        }
                        var noButtonFunction = function() {
                        }

                        showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                    }
                }
            }
        }
    }

    SelectLanguageDrawer {
        id: selectLanguageDrawer
    }
}
