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

        onActiveFocusChanged: {
            if(backButton.enabled && backButton.activeFocus) {
                listView.positionViewAtBeginning()
            }
        }
    }

    ListViewType {
        id: listView

        anchors.top: backButton.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        header: ColumnLayout {
            width: listView.width

            BaseHeaderType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("Application")
            }
        }

        model: 1 // fake model to force the ListView to be created without a model

        delegate: ColumnLayout { // TODO(CyAn84): add DelegateChooser when have migrated to 6.9

            width: listView.width

            SwitcherType {
                id: switcherAllowScreenshots

                visible: GC.isMobile()

                Layout.fillWidth: true
                Layout.margins: 16

                text: qsTr("Allow application screenshots")

                checked: SettingsController.isScreenshotsEnabled()
                onCheckedChanged: {
                    if (checked !== SettingsController.isScreenshotsEnabled()) {
                        SettingsController.toggleScreenshotsEnabled(checked)
                    }
                }
            }

            DividerType {
                visible: GC.isMobile()
            }

            LabelWithButtonType {
                id: labelWithButtonNotification

                visible: Qt.platform.os === "android" && !SettingsController.isNotificationPermissionGranted

                Layout.fillWidth: true

                text: qsTr("Enable notifications")
                descriptionText: qsTr("Enable notifications to show the VPN state in the status bar")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"

                clickedFunction: function() {
                    SettingsController.requestNotificationPermission()
                }
            }

            DividerType {
                visible: Qt.platform.os === "android" && !SettingsController.isNotificationPermissionGranted
            }

            SwitcherType {
                id: switcherAutoStart

                visible: !GC.isMobile()

                Layout.fillWidth: true
                Layout.margins: 16

                text: qsTr("Auto start")
                descriptionText: qsTr("Launch the application every time the device is starts")

                checked: SettingsController.isAutoStartEnabled()
                onCheckedChanged: {
                    if (checked !== SettingsController.isAutoStartEnabled()) {
                        SettingsController.toggleAutoStart(checked)
                    }
                }
            }

            DividerType {
                visible: !GC.isMobile()
            }

            SwitcherType {
                id: switcherAutoConnect

                visible: !GC.isMobile()

                Layout.fillWidth: true
                Layout.margins: 16

                text: qsTr("Auto connect")
                descriptionText: qsTr("Connect to VPN on app start")

                checked: SettingsController.isAutoConnectEnabled()
                onCheckedChanged: {
                    if (checked !== SettingsController.isAutoConnectEnabled()) {
                        SettingsController.toggleAutoConnect(checked)
                    }
                }
            }

            DividerType {
                visible: !GC.isMobile()
            }

            SwitcherType {
                id: switcherStartMinimized

                visible: !GC.isMobile()

                Layout.fillWidth: true
                Layout.margins: 16

                text: qsTr("Start minimized")
                descriptionText: qsTr("Launch application minimized (works with autostart option turned on)")

                enabled: switcherAutoStart.checked
                opacity: enabled ? 1.0 : 0.5

                checked: SettingsController.isStartMinimizedEnabled()
                onCheckedChanged: {
                    if (checked !== SettingsController.isStartMinimizedEnabled()) {
                        SettingsController.toggleStartMinimized(checked)
                    }
                }
            }

            DividerType {
                visible: !GC.isMobile()
            }
        }

        footer: ColumnLayout {

            width: listView.width

            LabelWithButtonType {
                id: labelWithButtonLanguage

                Layout.fillWidth: true

                text: qsTr("Language")
                descriptionText: LanguageModel.currentLanguageName
                rightImageSource: "qrc:/images/controls/chevron-right.svg"

                clickedFunction: function() {
                    selectLanguageDrawer.openTriggered()
                }
            }

            DividerType {}

            LabelWithButtonType {
                id: labelWithButtonLogging

                Layout.fillWidth: true

                text: qsTr("Logging")
                descriptionText: SettingsController.isLoggingEnabled ? qsTr("Enabled") : qsTr("Disabled")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"

                clickedFunction: function() {
                    PageController.goToPage(PageEnum.PageSettingsLogging)
                }
            }

            DividerType {}

            LabelWithButtonType {
                id: labelWithButtonReset
                
                Layout.fillWidth: true

                text: qsTr("Reset settings and remove all data from the application")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"
                textColor: AmneziaStyle.color.vibrantRed

                clickedFunction: function() {
                    var headerText = qsTr("Reset settings and remove all data from the application?")
                    var descriptionText = qsTr("All settings will be reset to default. All installed AmneziaVPN services will still remain on the server.")
                    var yesButtonText = qsTr("Continue")
                    var noButtonText = qsTr("Cancel")

                    var yesButtonFunction = function() {
                        if (ServersModel.isDefaultServerCurrentlyProcessed() && ConnectionController.isConnected) {
                            PageController.showNotificationMessage(qsTr("Cannot reset settings during active connection"))
                        } else
                        {
                            SettingsController.clearSettings()
                            PageController.goToPageHome()
                        }
                    }
                    var noButtonFunction = function() {
                    }

                    showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                }
            }

            DividerType {}
        }
    }

    SelectLanguageDrawer {
        id: selectLanguageDrawer

        width: root.width
        height: root.height
    }
}
