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

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20

        onFocusChanged: {
            console.debug("MOVE THIS LOGIC TO CPP!")
            if (activeFocus) {
                if (fl) {
                    fl.ensureVisible(this)
                }
            }
        }
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

                text: qsTr("Enable logs")

                checked: SettingsController.isLoggingEnabled
                
                onCheckedChanged: {
                    if (checked !== SettingsController.isLoggingEnabled) {
                        SettingsController.isLoggingEnabled = checked
                    }
                }

                parentFlickable: fl
            }

            DividerType {}

            LabelWithButtonType {
                // id: labelWithButton2
                Layout.fillWidth: true
                Layout.topMargin: -8

                text: qsTr("Clear logs")
                leftImageSource: "qrc:/images/controls/trash.svg"
                isSmallLeftImage: true

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
                        // if (!GC.isMobile()) {
                        //     focusItem.forceActiveFocus()
                        // }
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
                leftImageSource: "qrc:/images/controls/folder-open.svg"
                isSmallLeftImage: true

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
                leftImageSource: "qrc:/images/controls/save.svg"
                isSmallLeftImage: true

                onFocusChanged: {
                    console.debug("MOVE THIS LOGIC TO CPP!")
                    if (activeFocus) {
                        if (fl) {
                            fl.ensureVisible(this)
                        }
                    }
                }

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
                visible: !GC.isMobile()

                Layout.fillWidth: true
                Layout.topMargin: 32
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Service logs")
            }

            ParagraphTextType {
                visible: !GC.isMobile()

                Layout.fillWidth: true
                Layout.topMargin: 8
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                color: AmneziaStyle.color.mutedGray
                text: qsTr("AmneziaVPN-service logs")
            }

            LabelWithButtonType {
                // id: labelWithButton2

                visible: !GC.isMobile()

                Layout.fillWidth: true
                Layout.topMargin: -8
                Layout.bottomMargin: -8

                text: qsTr("Open logs folder")
                leftImageSource: "qrc:/images/controls/folder-open.svg"
                isSmallLeftImage: true

                onFocusChanged: {
                    console.debug("MOVE THIS LOGIC TO CPP!")
                    if (activeFocus) {
                        if (fl) {
                            fl.ensureVisible(this)
                        }
                    }
                }

                clickedFunction: function() {
                    SettingsController.openServiceLogsFolder()
                }
            }

            DividerType {
                visible: !GC.isMobile()
            }

            LabelWithButtonType {
                // id: labelWithButton2

                visible: !GC.isMobile()

                Layout.fillWidth: true
                Layout.topMargin: -8
                Layout.bottomMargin: -8

                text: qsTr("Export logs")
                leftImageSource: "qrc:/images/controls/save.svg"
                isSmallLeftImage: true

                onFocusChanged: {
                    console.debug("MOVE THIS LOGIC TO CPP!")
                    if (activeFocus) {
                        if (fl) {
                            fl.ensureVisible(this)
                        }
                    }
                }

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

            DividerType {
                visible: !GC.isMobile()
            }
        }
    }
}
