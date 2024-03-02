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

            HeaderType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("Application")
            }

            SwitcherType {
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

            SwitcherType {
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
                visible: !GC.isMobile()

                Layout.fillWidth: true
                Layout.margins: 16

                text: qsTr("Start minimized")
                descriptionText: qsTr("Launch application minimized")

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

            LabelWithButtonType {
                Layout.fillWidth: true

                text: qsTr("Language")
                descriptionText: LanguageModel.currentLanguageName
                rightImageSource: "qrc:/images/controls/chevron-right.svg"

                clickedFunction: function() {
                    selectLanguageDrawer.open()
                }
            }


            DividerType {}

            LabelWithButtonType {
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
                Layout.fillWidth: true

                text: qsTr("Reset settings and remove all data from the application")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"
                textColor: "#EB5757"

                clickedFunction: function() {
                    var headerText = qsTr("Reset settings and remove all data from the application?")
                    var descriptionText = qsTr("All settings will be reset to default. All installed AmneziaVPN services will still remain on the server.")
                    var yesButtonText = qsTr("Continue")
                    var noButtonText = qsTr("Cancel")

                    var yesButtonFunction = function() {
                        SettingsController.clearSettings()
                        PageController.replaceStartPage()
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
