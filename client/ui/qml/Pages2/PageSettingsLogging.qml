import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import QtCore

import PageEnum 1.0
import Style 1.0

import "../Controls2"
import "../Config"
import "../Components"
import "../Controls2/TextTypes"

PageType {
    id: root

    Connections {
        target: SettingsController

        function onLoggingStateChanged() {
            if (SettingsController.isLoggingEnabled) {
                var message = qsTr("Logging is enabled. Note that logs will be automatically \
disabled after 14 days, and all log files will be deleted.")
                PageController.showNotificationMessage(message)
            }
        }
    }

    defaultActiveFocusItem: focusItem

    Item {
        id: focusItem
        KeyNavigation.tab: backButton
    }

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20

        KeyNavigation.tab: switcher
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
            spacing: 0

            HeaderType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("Logging")
                descriptionText: qsTr("Enabling this function will save application's logs automatically. " +
                                      "By default, logging functionality is disabled. Enable log saving in case of application malfunction.")
            }

            SwitcherType {
                id: switcher
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Save logs")

                checked: SettingsController.isLoggingEnabled
                KeyNavigation.tab: openFolderButton
                onCheckedChanged: {
                    if (checked !== SettingsController.isLoggingEnabled) {
                        SettingsController.isLoggingEnabled = checked
                    }
                }
            }

            DividerType {}

            LabelWithButtonType {
                // id: labelWithButton2
                Layout.fillWidth: true
                Layout.topMargin: -8

                text: qsTr("Clear logs")
                rightImageSource: "qrc:/images/controls/trash.svg"

                // KeyNavigation.tab: labelWithButton3

                clickedFunction: function() {
                    var headerText = qsTr("Clear logs?")
                    var yesButtonText = qsTr("Continue")
                    var noButtonText = qsTr("Cancel")

                    var yesButtonFunction = function() {
                        PageController.showBusyIndicator(true)
                        SettingsController.clearLogs()
                        PageController.showBusyIndicator(false)
                        PageController.showNotificationMessage(qsTr("Logs have been cleaned up"))
                        if (!GC.isMobile()) {
                            focusItem.forceActiveFocus()
                        }
                    }
                    var noButtonFunction = function() {
                        if (!GC.isMobile()) {
                            focusItem.forceActiveFocus()
                        }
                    }

                    showQuestionDrawer(headerText, "", yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                }
            }

            ListItemTitleType {
                Layout.fillWidth: true
                Layout.topMargin: 8
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Client logs")
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.topMargin: 8
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                color: AmneziaStyle.color.mutedGray
                text: qsTr("AmneziaVPN logs")
            }

            LabelWithButtonType {
                // id: labelWithButton2
                Layout.fillWidth: true
                Layout.topMargin: -8
                Layout.bottomMargin: -8

                text: qsTr("Open logs folder")
                rightImageSource: "qrc:/images/controls/folder-open.svg"

                // KeyNavigation.tab: labelWithButton3

                clickedFunction: function() {
                    SettingsController.openLogsFolder()
                }
            }

            DividerType {}

            LabelWithButtonType {
                // id: labelWithButton2
                Layout.fillWidth: true
                Layout.topMargin: -8
                Layout.bottomMargin: -8

                text: qsTr("Export logs")
                rightImageSource: "qrc:/images/controls/save.svg"

                // KeyNavigation.tab: labelWithButton3

                clickedFunction: function() {
                    var fileName = ""
                    if (GC.isMobile()) {
                        fileName = "AmneziaVPN.log"
                    } else {
                        fileName = SystemController.getFileName(qsTr("Save"),
                                                                qsTr("Logs files (*.log)"),
                                                                StandardPaths.standardLocations(StandardPaths.DocumentsLocation) + "/AmneziaVPN",
                                                                true,
                                                                ".log")
                    }
                    if (fileName !== "") {
                        PageController.showBusyIndicator(true)
                        SettingsController.exportLogsFile(fileName)
                        PageController.showBusyIndicator(false)
                        PageController.showNotificationMessage(qsTr("Logs file saved"))
                    }
                }
            }

            DividerType {}

            ListItemTitleType {
                Layout.fillWidth: true
                Layout.topMargin: 32
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Service logs")
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.topMargin: 8
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                color: AmneziaStyle.color.mutedGray
                text: qsTr("AmneziaVPN-service logs")
            }

            LabelWithButtonType {
                // id: labelWithButton2
                Layout.fillWidth: true
                Layout.topMargin: -8
                Layout.bottomMargin: -8

                text: qsTr("Open logs folder")
                rightImageSource: "qrc:/images/controls/save.svg"

                // KeyNavigation.tab: labelWithButton3

                clickedFunction: function() {
                    SettingsController.openServiceLogsFolder()
                }
            }

            DividerType {}

            LabelWithButtonType {
                // id: labelWithButton2
                Layout.fillWidth: true
                Layout.topMargin: -8
                Layout.bottomMargin: -8

                text: qsTr("Export logs")
                rightImageSource: "qrc:/images/controls/save.svg"

                // KeyNavigation.tab: labelWithButton3

                clickedFunction: function() {
                    var fileName = ""
                    if (GC.isMobile()) {
                        fileName = "AmneziaVPN-service.log"
                    } else {
                        fileName = SystemController.getFileName(qsTr("Save"),
                                                                qsTr("Logs files (*.log)"),
                                                                StandardPaths.standardLocations(StandardPaths.DocumentsLocation) + "/AmneziaVPN-service",
                                                                true,
                                                                ".log")
                    }
                    if (fileName !== "") {
                        PageController.showBusyIndicator(true)
                        SettingsController.exportServiceLogsFile(fileName)
                        PageController.showBusyIndicator(false)
                        PageController.showNotificationMessage(qsTr("Logs file saved"))
                    }
                }
            }

            DividerType {}
        }
    }
}
