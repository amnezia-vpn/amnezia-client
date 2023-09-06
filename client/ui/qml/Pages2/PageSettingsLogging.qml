import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import QtCore

import PageEnum 1.0

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
            anchors.leftMargin: 16
            anchors.rightMargin: 16

            spacing: 16

            HeaderType {
                Layout.fillWidth: true

                headerText: qsTr("Logging")
            }

            SwitcherType {
                Layout.fillWidth: true
                Layout.topMargin: 16

                text: qsTr("Save logs")

                checked: SettingsController.isLoggingEnabled
                onCheckedChanged: {
                    if (checked !== SettingsController.isLoggingEnabled) {
                        SettingsController.isLoggingEnabled = checked
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true

                ColumnLayout {
                    Layout.alignment: Qt.AlignBaseline
                    Layout.preferredWidth: root.width / 3

                    ImageButtonType {
                        Layout.alignment: Qt.AlignHCenter

                        implicitWidth: 56
                        implicitHeight: 56

                        image: "qrc:/images/controls/folder-open.svg"

                        onClicked: SettingsController.openLogsFolder()
                    }

                    CaptionTextType {
                        horizontalAlignment: Text.AlignHCenter
                        Layout.fillWidth: true

                        text: qsTr("Open folder with logs")
                        color: "#D7D8DB"
                    }
                }

                ColumnLayout {
                    Layout.alignment: Qt.AlignBaseline
                    Layout.preferredWidth: root.width / 3

                    ImageButtonType {
                        Layout.alignment: Qt.AlignHCenter

                        implicitWidth: 56
                        implicitHeight: 56

                        image: "qrc:/images/controls/save.svg"

                        onClicked: {
                            if (Qt.platform.os === "ios") {
                                SettingsController.exportLogsFile("AmneziaVPN.log")
                            } else {
                                fileDialog.open()
                            }
                        }

                        FileDialog {
                            id: fileDialog
                            acceptLabel: qsTr("Save logs")
                            nameFilters: [ "Logs files (*.log)" ]
                            fileMode: FileDialog.SaveFile

                            currentFile: StandardPaths.standardLocations(StandardPaths.DocumentsLocation) + "/AmneziaVPN"
                            defaultSuffix: ".log"
                            onAccepted: {
                                SettingsController.exportLogsFile(fileDialog.currentFile.toString())
                            }
                        }
                    }

                    CaptionTextType {
                        horizontalAlignment: Text.AlignHCenter
                        Layout.fillWidth: true

                        text: qsTr("Save logs to file")
                        color: "#D7D8DB"
                    }
                }

                ColumnLayout {
                    Layout.alignment: Qt.AlignBaseline
                    Layout.preferredWidth: root.width / 3

                    ImageButtonType {
                        Layout.alignment: Qt.AlignHCenter

                        implicitWidth: 56
                        implicitHeight: 56

                        image: "qrc:/images/controls/delete.svg"

                        onClicked: function() {
                            questionDrawer.headerText = qsTr("Clear logs?")
                            questionDrawer.yesButtonText = qsTr("Continue")
                            questionDrawer.noButtonText = qsTr("Cancel")

                            questionDrawer.yesButtonFunction = function() {
                                questionDrawer.visible = false
                                PageController.showBusyIndicator(true)
                                SettingsController.clearLogs()
                                PageController.showBusyIndicator(false)
                                PageController.showNotificationMessage(qsTr("Logs have been cleaned up"))
                            }
                            questionDrawer.noButtonFunction = function() {
                                questionDrawer.visible = false
                            }
                            questionDrawer.visible = true
                        }
                    }

                    CaptionTextType {
                        horizontalAlignment: Text.AlignHCenter
                        Layout.fillWidth: true

                        text: qsTr("Clear logs")
                        color: "#D7D8DB"
                    }
                }
            }

            QuestionDrawer {
                id: questionDrawer
            }
        }
    }
}
