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
                    Layout.preferredWidth: GC.isMobile() ? 0 : root.width / 3
                    visible: !GC.isMobile()

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
                    Layout.preferredWidth: root.width / ( GC.isMobile() ? 2 : 3 )

                    ImageButtonType {
                        Layout.alignment: Qt.AlignHCenter

                        implicitWidth: 56
                        implicitHeight: 56

                        image: "qrc:/images/controls/save.svg"

                        onClicked: {
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

                    CaptionTextType {
                        horizontalAlignment: Text.AlignHCenter
                        Layout.fillWidth: true

                        text: qsTr("Save logs to file")
                        color: "#D7D8DB"
                    }
                }

                ColumnLayout {
                    Layout.alignment: Qt.AlignBaseline
                    Layout.preferredWidth: root.width / ( GC.isMobile() ? 2 : 3 )

                    ImageButtonType {
                        Layout.alignment: Qt.AlignHCenter

                        implicitWidth: 56
                        implicitHeight: 56

                        image: "qrc:/images/controls/delete.svg"

                        onClicked: function() {
                            var headerText = qsTr("Clear logs?")
                            var yesButtonText = qsTr("Continue")
                            var noButtonText = qsTr("Cancel")

                            var yesButtonFunction = function() {
                                PageController.showBusyIndicator(true)
                                SettingsController.clearLogs()
                                PageController.showBusyIndicator(false)
                                PageController.showNotificationMessage(qsTr("Logs have been cleaned up"))
                            }
                            var noButtonFunction = function() {
                            }

                            showQuestionDrawer(headerText, "", yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
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
        }
    }
}
