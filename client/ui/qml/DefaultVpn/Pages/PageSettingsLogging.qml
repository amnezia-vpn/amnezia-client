import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QtCore

import PageEnum 1.0
import Config 1.0

import "../Components"
import "../Controls"
import "../Controls/TextTypes"

Page {
    id: root

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 8
        anchors.topMargin: 8

        RowLayout {
            Layout.fillWidth: true

            WhiteButtonNoBorder {
                id: backButton
                imageSource: "qrc:/images/controls/arrow-left.svg"
                onClicked: PageController.closePage()
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 8
            Layout.rightMargin: 8
            Layout.topMargin: 8

            RowLayout {
                Layout.fillWidth: true

                Header1TextType {
                    Layout.fillWidth: true

                    text: qsTr("Logging")

                    horizontalAlignment: Qt.AlignLeft
                    verticalAlignment: Qt.AlignVCenter
                }

                SwitcherType {
                    id: switcher

                    Layout.fillWidth: true
                    Layout.topMargin: 4
                    Layout.bottomMargin: 4
                    Layout.rightMargin: 4

                    checked: SettingsController.isLoggingEnabled
                    
                    onCheckedChanged: {
                        if (checked !== SettingsController.isLoggingEnabled) {
                            SettingsController.isLoggingEnabled = checked
                        }
                    }
                }
            }

            MediumTextType {
                Layout.fillWidth: true
                Layout.topMargin: 16
                text: qsTr("In case of application failures, enable logging to find the problem")

                horizontalAlignment: Qt.AlignLeft
                verticalAlignment: Qt.AlignVCenter
            }

            WhiteButtonWithBorder {
                Layout.fillWidth: true
                Layout.topMargin: 40
                
                text: qsTr("Save logs to file")

                onClicked: function() {
                    var fileName = ""
                    if (DeviceInfo.isMobile()) {
                        fileName = "DefaultVPN.log"
                    } else {
                        fileName = SystemController.getFileName(qsTr("Save"),
                                                                qsTr("Logs files (*.log)"),
                                                                StandardPaths.standardLocations(StandardPaths.DocumentsLocation) + "/DefaultVPN",
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

            WhiteButtonWithBorder {
                Layout.fillWidth: true
                Layout.topMargin: 16
                
                text: qsTr("Open logs")

                onClicked: function() {
                    SettingsController.openLogsFolder()
                }
            }

            Item {
                Layout.fillHeight: true
            }
        }
    }
} 