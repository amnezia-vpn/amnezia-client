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
import "../Controls2/TextTypes"

PageType {
    id: root

    Connections {
        target: SettingsController

        function onChangeSettingsErrorOccurred(errorMessage) {
            PageController.showErrorMessage(errorMessage)
        }

        function onRestoreBackupFinished() {
            PageController.showNotificationMessage(qsTr("Settings restored from backup file"))
            PageController.goToPageHome()
        }

        function onImportBackupFromOutside(filePath) {
            restoreBackup(filePath)
        }
    }

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

                headerText: qsTr("Back up your configuration")
                descriptionText: qsTr("You can save your settings to a backup file to restore them the next time you install the application.")
            }

            WarningType {
                Layout.topMargin: 16
                Layout.fillWidth: true

                textString: qsTr("The backup will contain your passwords and private keys for all servers added " +
                                            "to AmneziaVPN. Keep this information in a secure place.")

                iconPath: "qrc:/images/controls/alert-circle.svg"
            }

            BasicButtonType {
                id: makeBackupButton
                Layout.fillWidth: true
                Layout.topMargin: 14

                text: qsTr("Make a backup")

                parentFlickable: fl

                clickedFunc: function() {
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

            BasicButtonType {
                id: restoreBackupButton
                Layout.fillWidth: true
                Layout.topMargin: -8

                defaultColor: AmneziaStyle.color.transparent
                hoveredColor: AmneziaStyle.color.translucentWhite
                pressedColor: AmneziaStyle.color.sheerWhite
                disabledColor: AmneziaStyle.color.mutedGray
                textColor: AmneziaStyle.color.paleGray
                borderWidth: 1

                text: qsTr("Restore from backup")

                parentFlickable: fl

                clickedFunc: function() {
                    var filePath = SystemController.getFileName(qsTr("Open backup file"),
                                                                qsTr("Backup files (*.backup)"))
                    if (filePath !== "") {
                        restoreBackup(filePath)
                    }
                }
            }
        }
    }

    function restoreBackup(filePath) {
        var headerText = qsTr("Import settings from a backup file?")
        var descriptionText = qsTr("All current settings will be reset");
        var yesButtonText = qsTr("Continue")
        var noButtonText = qsTr("Cancel")

        var yesButtonFunction = function() {
            if (ConnectionController.isConnected) {
                PageController.showNotificationMessage(qsTr("Cannot restore backup settings during active connection"))
            } else {
                PageController.showBusyIndicator(true)
                SettingsController.restoreAppConfig(filePath)
                PageController.showBusyIndicator(false)
            }
        }
        var noButtonFunction = function() {
        }

        showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
    }
}
