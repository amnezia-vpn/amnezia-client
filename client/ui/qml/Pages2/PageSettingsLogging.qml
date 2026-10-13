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

    Rectangle {
        anchors.fill: parent
        color: AmneziaStyle.color.backgroundBase
    }

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20 + PageController.safeAreaTopMargin

        onFocusChanged: {
            if (this.activeFocus) {
                flickable.contentY = 0
            }
        }
    }

    FlickableType {
        id: flickable

        anchors.top: backButton.bottom
        anchors.bottom: parent.bottom
        contentHeight: content.implicitHeight + content.anchors.topMargin + 16 + PageController.safeAreaBottomMargin

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 12
            anchors.leftMargin: 16
            anchors.rightMargin: 16

            spacing: 24

            ColumnLayout {
                Layout.fillWidth: true

                spacing: 8

                AppH1TextType {
                    Layout.fillWidth: true

                    text: qsTr("Logging")
                }

                AppTextType {
                    Layout.fillWidth: true

                    color: AmneziaStyle.color.surfaceInverse
                    text: qsTr("Logs help support figure out what's wrong")
                }
            }

            CardGroupType {
                Layout.fillWidth: true

                ActionRowType {
                    id: loggingRow

                    Layout.fillWidth: true
                    Layout.minimumHeight: 88

                    text: qsTr("Record logs")
                    descriptionText: qsTr("Recording turns off after 14 days and logs are deleted")
                    showDivider: false

                    clickedFunction: function() {
                        switcher.toggle()
                        switcher.toggled()
                    }

                    SwitcherType {
                        id: switcher

                        width: indicatorWidth
                        height: indicatorHeight

                        indicatorWidth: 50
                        indicatorHeight: 30
                        checkedInnerCircleSize: 20
                        checkedInnerCircleMargin: 5

                        checkedIndicatorColor: AmneziaStyle.color.switchTrackChecked
                        checkedIndicatorBorderColor: AmneziaStyle.color.switchTrackChecked
                        checkedInnerCircleColor: AmneziaStyle.color.switchThumbChecked
                        defaultIndicatorBorderColor: AmneziaStyle.color.textTertiary

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

                AppTextType {
                    Layout.fillWidth: true

                    color: AmneziaStyle.color.textTertiary
                    text: qsTr("Logs")
                }

                CardGroupType {
                    Layout.fillWidth: true

                    ActionRowType {
                        id: tunnelLogsRow

                        Layout.fillWidth: true

                        text: qsTr("Tunnel")
                        descriptionText: qsTr("Connection events") + " · " + LogsController.tunnelLogSize
                        rightImageSource: "qrc:/images/controls/chevron-right.svg"

                        clickedFunction: function() {
                            LogsController.viewerStream = "tunnel"
                            PageController.goToPage(PageEnum.PageSettingsLogViewer)
                        }
                    }

                    ActionRowType {
                        id: appLogsRow

                        Layout.fillWidth: true

                        text: qsTr("Application")
                        descriptionText: qsTr("App events") + " · " + LogsController.appLogSize
                        rightImageSource: "qrc:/images/controls/chevron-right.svg"
                        showDivider: false

                        clickedFunction: function() {
                            LogsController.viewerStream = "app"
                            PageController.goToPage(PageEnum.PageSettingsLogViewer)
                        }
                    }
                }
            }

            CardGroupType {
                Layout.fillWidth: true

                ActionRowType {
                    id: saveAllRow

                    Layout.fillWidth: true

                    text: qsTr("Save all logs")
                    descriptionText: qsTr(".zip archive with both logs")
                    leftImageSource: "qrc:/images/controls/save.svg"

                    clickedFunction: function() {
                        root.saveAll()
                    }
                }

                ActionRowType {
                    id: clearAllRow

                    Layout.fillWidth: true

                    text: qsTr("Clear all logs")
                    textColor: AmneziaStyle.color.accentDanger
                    leftImageSource: "qrc:/images/controls/trash-2.svg"
                    leftImageColor: AmneziaStyle.color.accentDanger
                    showDivider: false

                    clickedFunction: function() {
                        clearAllDialog.open()
                    }
                }
            }
        }
    }
}
