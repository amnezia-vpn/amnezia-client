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

    property bool exportPending: false
    property bool exportIsShare: false

    Component.onCompleted: {
        LogsController.refreshSizes()
    }

    onVisibleChanged: {
        if (visible) {
            LogsController.refreshSizes()
        }
    }

    function runExport(isShare, exportFunction) {
        root.exportPending = true
        root.exportIsShare = isShare
        exportFunction()
    }

    function saveAll() {
        if (LogsController.canShare) {
            var saveFunction = function() {
                root.runExport(false, function() {
                    LogsController.saveAll(LogsController.defaultFileName("all"))
                })
            }
            var shareFunction = function() {
                root.runExport(true, function() {
                    LogsController.shareAll()
                })
            }
            showQuestionDrawer(qsTr("Save all logs"),
                               qsTr("Save the archive on this device or send it to another app."),
                               qsTr("Save to file…"), qsTr("Share…"),
                               saveFunction, shareFunction)
            return
        }

        var fileName = ""
        if (GC.isMobile()) {
            fileName = LogsController.defaultFileName("all")
        } else {
            fileName = SystemController.getFileName(qsTr("Save"),
                                                    qsTr("Archive files (*.zip)"),
                                                    StandardPaths.standardLocations(StandardPaths.DocumentsLocation) + "/" + LogsController.defaultFileName("all"),
                                                    true,
                                                    ".zip")
        }
        if (fileName !== "") {
            root.runExport(false, function() {
                LogsController.saveAll(fileName)
            })
        }
    }

    Connections {
        target: LogsController

        function onBusyChanged() {
            PageController.showBusyIndicator(LogsController.busy)
        }

        function onExportFinished(success) {
            if (!root.exportPending) {
                return
            }
            root.exportPending = false
            if (!success) {
                PageController.showNotificationMessage(qsTr("Failed to save logs"))
            } else if (!root.exportIsShare) {
                PageController.showNotificationMessage(qsTr("Logs file saved"))
            }
        }
    }

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20 + PageController.safeAreaTopMargin

        onFocusChanged: {
            if (this.activeFocus) {
                listView.positionViewAtBeginning()
            }
        }
    }

    ListViewType {
        id: listView

        anchors.top: backButton.bottom
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.left: parent.left

        header: ColumnLayout {
            width: listView.width

            BaseHeaderType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("Logging")
                descriptionText: qsTr("Enabling this function will save application's logs automatically. " +
                                      "By default, logging functionality is disabled. Enable log saving in case of application malfunction.")
            }

            LabelTextType {
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Collection")
            }

            SwitcherType {
                id: switcher

                Layout.fillWidth: true
                Layout.topMargin: 8
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Enable logs")
                descriptionText: qsTr("Off: the app and the tunnel stop writing logs. Existing logs stay readable.")

                checked: SettingsController.isLoggingEnabled

                onToggled: function() {
                    if (checked !== SettingsController.isLoggingEnabled) {
                        SettingsController.isLoggingEnabled = checked
                    }
                }
            }

            DividerType {}
        }

        model: 1

        delegate: ColumnLayout {
            width: listView.width

            spacing: 0

            LabelTextType {
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Tunnel")
            }

            LabelWithButtonType {
                id: tunnelLogsButton

                Layout.fillWidth: true

                text: qsTr("Tunnel")
                descriptionText: ((!GC.isMobile() && !IsMacOsNeBuild) ? qsTr("Service events")
                                                                      : qsTr("Network extension and VPN service events"))
                                 + " · " + LogsController.tunnelLogSize
                rightImageSource: "qrc:/images/controls/chevron-right.svg"

                clickedFunction: function() {
                    LogsController.viewerStream = "tunnel"
                    PageController.goToPage(PageEnum.PageSettingsLogViewer)
                }
            }

            DividerType {}

            LabelTextType {
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Application")
            }

            LabelWithButtonType {
                id: appLogsButton

                Layout.fillWidth: true

                text: qsTr("Application")
                descriptionText: qsTr("Client-side events") + " · " + LogsController.appLogSize
                rightImageSource: "qrc:/images/controls/chevron-right.svg"

                clickedFunction: function() {
                    LogsController.viewerStream = "app"
                    PageController.goToPage(PageEnum.PageSettingsLogViewer)
                }
            }

            DividerType {}

            BasicButtonType {
                id: saveAllButton

                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Save all logs (.zip)")

                clickedFunc: function() {
                    root.saveAll()
                }
            }

            LabelWithButtonType {
                id: clearAllButton

                Layout.fillWidth: true
                Layout.topMargin: 8

                text: qsTr("Clear all logs")
                textColor: AmneziaStyle.color.vibrantRed
                leftImageSource: "qrc:/images/controls/trash.svg"
                isSmallLeftImage: true

                clickedFunction: function() {
                    var headerText = qsTr("Clear all logs?")
                    var descriptionText = qsTr("Application and tunnel logs will be deleted. This can't be undone.")
                    var yesButtonText = qsTr("Continue")
                    var noButtonText = qsTr("Cancel")

                    var yesButtonFunction = function() {
                        PageController.showBusyIndicator(true)
                        SettingsController.clearLogs()
                        LogsController.refreshSizes()
                        PageController.showBusyIndicator(false)
                        PageController.showNotificationMessage(qsTr("Logs have been cleaned up"))
                    }

                    var noButtonFunction = function() {

                    }

                    showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                }
            }
        }
    }
}
