import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

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

    Component.onCompleted: {
        LogsController.refreshSizes()
    }

    onVisibleChanged: {
        if (visible) {
            LogsController.refreshSizes()
        }
    }

    function saveAll() {
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
            root.exportPending = true
            LogsController.saveAll(fileName)
        }
    }

    function clearAll() {
        PageController.showBusyIndicator(true)
        SettingsController.clearLogs()
        LogsController.refreshSizes()
        PageController.showBusyIndicator(false)
        PageController.showNotificationMessage(qsTr("Logs have been cleaned up"))
    }

    AlertDialogType {
        id: clearAllDialog

        headerText: qsTr("Clear all logs?")
        descriptionText: qsTr("App and tunnel logs will be deleted from the device and can't be restored")
        confirmButtonText: qsTr("Clear")
        role: AlertDialogType.Destructive

        onConfirmed: {
            root.clearAll()
        }
    }

    Connections {
        target: LogsController

        function onBusyChanged() {
            PageController.showBusyIndicator(LogsController.busy)
        }

        function onExportFinished(success, cancelled) {
            if (!root.exportPending) {
                return
            }
            root.exportPending = false
            if (success) {
                PageController.showNotificationMessage(qsTr("Logs file saved"))
            } else if (!cancelled) {
                PageController.showNotificationMessage(qsTr("Failed to save logs. Try again later"))
            }
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

                title: qsTr("Logging")
                description: qsTr("Logs help support figure out what's wrong")
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 24 + PageController.safeAreaBottomMargin

                spacing: 24

                ListGroupType {
                    Layout.fillWidth: true

                    ListItemType {
                        id: loggingRow

                        Layout.fillWidth: true

                        text: qsTr("Record logs")
                        subtitle: qsTr("Recording turns off after 14 days and logs are deleted")

                        onClicked: {
                            switcher.toggle()
                            switcher.toggled()
                        }

                        SwitchType {
                            id: switcher

                            focusPolicy: Qt.NoFocus

                            checked: SettingsController.isLoggingEnabled

                            onToggled: function() {
                                if (checked !== SettingsController.isLoggingEnabled) {
                                    SettingsController.isLoggingEnabled = checked
                                }
                            }
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true

                    spacing: 12

                    BodyTextType {
                        Layout.fillWidth: true

                        text: qsTr("Logs")
                        color: AmneziaStyle.color.textTertiary
                    }

                    ListGroupType {
                        Layout.fillWidth: true

                        ListItemType {
                            id: tunnelLogsRow

                            Layout.fillWidth: true

                            text: qsTr("Tunnel")
                            subtitle: qsTr("Connection events") + " · " + LogsController.tunnelLogSize
                            trailingIconSource: "qrc:/images/controls/chevron-right.svg"

                            onClicked: {
                                LogsController.viewerStream = "tunnel"
                                PageController.goToPage(PageEnum.PageSettingsLogViewer)
                            }
                        }

                        ListItemType {
                            id: appLogsRow

                            Layout.fillWidth: true

                            text: qsTr("Application")
                            subtitle: qsTr("App events") + " · " + LogsController.appLogSize
                            trailingIconSource: "qrc:/images/controls/chevron-right.svg"

                            onClicked: {
                                LogsController.viewerStream = "app"
                                PageController.goToPage(PageEnum.PageSettingsLogViewer)
                            }
                        }
                    }
                }

                ListGroupType {
                    Layout.fillWidth: true

                    ListItemType {
                        id: saveAllRow

                        Layout.fillWidth: true

                        enabled: LogsController.hasLogs
                        text: qsTr("Save all logs")
                        subtitle: qsTr(".zip archive with both logs")
                        icon.source: "qrc:/images/controls/save.svg"

                        onClicked: {
                            root.saveAll()
                        }
                    }

                    ListItemType {
                        id: clearAllRow

                        Layout.fillWidth: true

                        enabled: LogsController.hasLogs
                        text: qsTr("Clear all logs")
                        destructive: true
                        icon.source: "qrc:/images/controls/trash-2.svg"
                        icon.color: AmneziaStyle.color.statusDestructive

                        onClicked: {
                            clearAllDialog.open()
                        }
                    }
                }
            }
        }
    }
}
