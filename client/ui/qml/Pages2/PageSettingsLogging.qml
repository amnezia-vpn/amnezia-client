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

            SwitcherType {
                id: switcher

                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Enable logs")

                checked: SettingsController.isLoggingEnabled
                
                onToggled: function() {
                    if (checked !== SettingsController.isLoggingEnabled) {
                        SettingsController.isLoggingEnabled = checked
                    }
                }
            }

            DividerType {}

            LabelWithButtonType {
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
                    }

                    var noButtonFunction = function() {

                    }

                    showQuestionDrawer(headerText, "", yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                }
            }

            DividerType {
                visible: networkDiagnosticsAvailable
            }

            LabelWithButtonType {
                Layout.fillWidth: true
                Layout.topMargin: -8

                visible: networkDiagnosticsAvailable

                text: qsTr("Run network diagnostics")
                leftImageSource: "qrc:/images/controls/scan-line.svg"
                isSmallLeftImage: true

                clickedFunction: function() {
                    var headerText = qsTr("Run network diagnostics?")
                    var descriptionText = qsTr("This collects local network configuration (adapters, routes, DNS and similar) and saves it as a new Amnezia-network log file, named with the current date and time. It takes a few seconds and does not open any window.")
                    var yesButtonText = qsTr("Continue")
                    var noButtonText = qsTr("Cancel")

                    var yesButtonFunction = function() {
                        PageController.showBusyIndicator(true)
                        var success = SettingsController.runNetworkDiagnostics()
                        PageController.showBusyIndicator(false)
                        if (success) {
                            PageController.showNotificationMessage(qsTr("Network diagnostics saved"))
                        } else {
                            PageController.showNotificationMessage(qsTr("Network diagnostics failed"))
                        }
                    }

                    var noButtonFunction = function() {

                    }

                    showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                }
            }
        }

        model: logTypes

        snapMode: ListView.SnapOneItem

        delegate: ColumnLayout {
            id: delegateContent

            width: listView.width

            enabled: isVisible

            ListItemTitleType {
                Layout.fillWidth: true
                Layout.topMargin: 8
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: title
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.topMargin: 8
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                color: AmneziaStyle.color.mutedGray

                text: description
            }

            LabelWithButtonType {
                Layout.fillWidth: true
                Layout.topMargin: -8
                Layout.bottomMargin: -8

                visible: !GC.isMobile()

                text: qsTr("Open logs folder")
                leftImageSource: "qrc:/images/controls/folder-open.svg"
                isSmallLeftImage: true

                clickedFunction: openLogsHandler
            }

            DividerType {}

            LabelWithButtonType {
                Layout.fillWidth: true
                Layout.topMargin: -8
                Layout.bottomMargin: -8

                text: qsTr("Export logs")
                leftImageSource: "qrc:/images/controls/save.svg"
                isSmallLeftImage: true

                clickedFunction: exportLogsHandler
            }

            DividerType {}
        }
    }

    readonly property bool networkDiagnosticsAvailable: Qt.platform.os === "windows" || Qt.platform.os === "linux"
            || Qt.platform.os === "osx" || Qt.platform.os === "android" || Qt.platform.os === "ios"

    // Mobile platforms and macOS Network-Extension builds have no separate background
    // service process, so there are no service logs to show. Network diagnostics on
    // mobile is collected in-process (not via a service), so it's independent of that
    // and only gated by networkDiagnosticsAvailable above.
    property list<QtObject> logTypes: {
        if (IsMacOsNeBuild) {
            return [clientLogs]
        } else if (GC.isMobile()) {
            return networkDiagnosticsAvailable ? [clientLogs, networkDiagnosticsLog] : [clientLogs]
        } else if (networkDiagnosticsAvailable) {
            return [clientLogs, serviceLogs, networkDiagnosticsLog]
        } else {
            return [clientLogs, serviceLogs]
        }
    }

    QtObject {
        id: clientLogs

        readonly property string title: qsTr("Client logs")
        readonly property string description: qsTr("AmneziaVPN logs")
        readonly property bool isVisible: true
        readonly property var openLogsHandler: function() {
            SettingsController.openLogsFolder()
        }
        readonly property var exportLogsHandler: function() {
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

    QtObject {
        id: serviceLogs

        readonly property string title: qsTr("Service logs")
        readonly property string description: qsTr("AmneziaVPN-service logs")
        readonly property bool isVisible: !GC.isMobile() && !IsMacOsNeBuild
        readonly property var openLogsHandler: function() {
            SettingsController.openServiceLogsFolder()
        }
        readonly property var exportLogsHandler: function() {
            var fileName = ""
            fileName = SystemController.getFileName(qsTr("Save"),
                                                    qsTr("Logs files (*.log)"),
                                                    StandardPaths.standardLocations(StandardPaths.DocumentsLocation) + "/AmneziaVPN-service",
                                                    true,
                                                    ".log")
            if (fileName !== "") {
                PageController.showBusyIndicator(true)
                SettingsController.exportServiceLogsFile(fileName)
                PageController.showBusyIndicator(false)
                PageController.showNotificationMessage(qsTr("Logs file saved"))
            }
        }
    }

    QtObject {
        id: networkDiagnosticsLog

        readonly property string title: qsTr("Network diagnostics")
        readonly property string description: qsTr("Local network configuration snapshot collected via the background service")
        readonly property bool isVisible: true
        readonly property var openLogsHandler: function() {
            SettingsController.openLogsFolder()
        }
        readonly property var exportLogsHandler: function() {
            var fileName = ""
            if (GC.isMobile()) {
                var timestamp = Qt.formatDateTime(new Date(), "yyyy-MM-dd_HH-mm-ss")
                fileName = "Amnezia-network-" + timestamp + ".log"
            } else {
                var timestamp = Qt.formatDateTime(new Date(), "yyyy-MM-dd_HH-mm-ss")
                fileName = SystemController.getFileName(qsTr("Save"),
                                                        qsTr("Logs files (*.log)"),
                                                        StandardPaths.standardLocations(StandardPaths.DocumentsLocation) + "/Amnezia-network-" + timestamp,
                                                        true,
                                                        ".log")
            }
            if (fileName !== "") {
                PageController.showBusyIndicator(true)
                SettingsController.exportNetworkDiagnosticsFile(fileName)
                PageController.showBusyIndicator(false)
                PageController.showNotificationMessage(qsTr("Logs file saved"))
            }
        }
    }
}
