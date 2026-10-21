import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import QtCore

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Config"
import "../Components"

PageType {
    id: root

    property bool isRestoringBackup: false

    Connections {
        target: SettingsController

        function onRestoreBackupFinished() {
            PageController.showNotificationMessage(qsTr("Settings restored from backup file"))
            PageController.goToPageHome()
        }

        function onImportBackupFromOutside(filePath) {
            restoreBackup(filePath)
        }
    }

    FlickableType {
        id: flickable

        anchors.top: parent.top
        anchors.bottom: buttons.top

        contentHeight: content.implicitHeight

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 0

            NavigationBarType {
                Layout.fillWidth: true

                title: qsTr("Back up your configuration")
                description: qsTr("You can save your settings to a backup file to restore them the next time you install the application.")
            }

            NotificationType {
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 16

                text: qsTr("The backup will contain your passwords and private keys for all servers added " +
                                 "to AmneziaVPN. Keep this information in a secure place.")
            }
        }
    }

    ColumnLayout {
        id: buttons

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        anchors.bottomMargin: 8 + PageController.safeAreaBottomMargin

        spacing: 12

        ButtonType {
            id: makeBackupButton

            Layout.fillWidth: true

            text: qsTr("Make a backup")

            onClicked: {
                var fileName = ""
                if (GC.isMobile()) {
                    fileName = "AmneziaVPN.backup"
                } else {
                    fileName = SystemController.getFileName(qsTr("Save backup file"),
                                                            qsTr("Backup files (*.backup)"),
                                                            StandardPaths.standardLocations(StandardPaths.DocumentsLocation) + "/AmneziaVPN",
                                                            true,
                                                            ".backup")
                }
                if (fileName !== "") {
                    PageController.showBusyIndicator(true)
                    SettingsController.backupAppConfig(fileName)
                    PageController.showBusyIndicator(false)
                    PageController.showNotificationMessage(qsTr("Backup file saved"))
                }
            }
        }

        ButtonType {
            id: restoreBackupButton

            Layout.fillWidth: true

            variant: ButtonType.Secondary
            enabled: !root.isRestoringBackup

            text: qsTr("Restore from backup")

            onClicked: {
                if (root.isRestoringBackup) {
                    return
                }
                var filePath = SystemController.getFileName(qsTr("Open backup file"),
                                                            qsTr("Backup files (*.backup)"))
                if (filePath !== "") {
                    restoreBackup(filePath)
                }
            }
        }
    }

    function restoreBackup(filePath) {
        if (root.isRestoringBackup) {
            return
        }

        var headerText = qsTr("Import settings from a backup file?")
        var descriptionText = qsTr("All current settings will be reset");
        var yesButtonText = qsTr("Continue")
        var noButtonText = qsTr("Cancel")

        var yesButtonFunction = function() {
            if (ConnectionController.isConnected) {
                PageController.showNotificationMessage(qsTr("Cannot restore backup settings during active connection"))
            } else {
                root.isRestoringBackup = true
                PageController.showBusyIndicator(true)
                Qt.callLater(function() {
                    SettingsController.restoreAppConfig(filePath)
                    PageController.showBusyIndicator(false)
                    root.isRestoringBackup = false
                })
            }
        }
        var noButtonFunction = function() {
        }

        showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
    }
}
