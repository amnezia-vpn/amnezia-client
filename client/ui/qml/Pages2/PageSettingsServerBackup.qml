import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import ProtocolEnum 1.0
import Style 1.0

import "../Controls2"
import "../Controls2/TextTypes"
import "../Components"
import "../Config"

PageType {
    id: root

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20 + SettingsController.safeAreaTopMargin

        onActiveFocusChanged: {
            if(backButton.enabled && backButton.activeFocus) {
                flickable.contentY = 0
            }
        }
    }

    FlickableType {
        id: flickable

        anchors.top: backButton.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        contentHeight: contentColumn.implicitHeight

        ColumnLayout {
            id: contentColumn
            width: flickable.width

            spacing: 16

            BaseHeaderType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 8
                
                headerText: qsTr("Backup")
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                
                text: qsTr("Local copy of VPN protocols, services, all server settings and users.")
                color: AmneziaStyle.color.mutedGray
            }

            Text {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: -8
                
                text: qsTr("More about backups")
                color: AmneziaStyle.color.goldenApricot
                font.pixelSize: 14
                
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        // TODO: Open help page or show more info
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 16
                spacing: 12

                BasicButtonType {
                    Layout.fillWidth: true
                    text: qsTr("Create backup")
                    
                    clickedFunc: function() {
                        createBackup(true)
                    }
                }

                BasicButtonType {
                    Layout.fillWidth: true
                    text: qsTr("Restore from backup")
                    defaultColor: AmneziaStyle.color.transparent
                    hoveredColor: Qt.rgba(1, 1, 1, 0.08)
                    pressedColor: Qt.rgba(1, 1, 1, 0.12)
                    disabledColor: AmneziaStyle.color.mutedGray
                    textColor: AmneziaStyle.color.goldenApricot
                    borderWidth: 1

                    clickedFunc: function() {
                        restoreBackup()
                    }
                }
            }
        }
    }

    function createBackup(shouldDownload) {
        // Default shouldDownload = true
        var downloadAfterCreate = (shouldDownload !== undefined) ? shouldDownload : true
        
        var headerText = downloadAfterCreate ? 
            qsTr("Create backup and download to device?") :
            qsTr("Create server configuration backup?")
        var descriptionText = downloadAfterCreate ?
            qsTr("Backup will be created on server and automatically downloaded to your device") :
            qsTr("This will create a backup of your server containers configuration on the server")
        var yesButtonText = qsTr("Create")
        var noButtonText = qsTr("Cancel")

        var yesButtonFunction = function() {
            PageController.showBusyIndicator(true)
            // Call C++ method that manages download and delete automatically
            ServersBackupController.createBackupWithDownload(downloadAfterCreate, true)
        }
        var noButtonFunction = function() {}

        showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
    }

    function restoreBackup() {
        var filter = GC.isMobile() ? "*.gz *.tgz *.tar.gz" : "Backup files (*.tar.gz *.backup *.tgz *.gz)"
        var localPath = SystemController.getFileName(
            qsTr("Select Backup to Restore"), 
            filter,
            "",
            false,
            ""
        )
        
        if (!localPath || localPath.length === 0) {
            return
        }
        
        // Get file information via C++
        var fileInfo = ServersBackupController.getBackupFileInfo(localPath)
        var fileName = fileInfo.fileName || "backup.tgz"
        var serverIp = fileInfo.serverIp || ""
        
        // If IP not found in filename, use current server
        if (!serverIp || serverIp.length === 0) {
            serverIp = ServersModel.getProcessedServerData("hostName") || ""
        }
        
        var serverName = ServersModel.getProcessedServerData("name") || qsTr("Server")
        
        // Open restore mode selection page
        var parentItem = root.parent
        while (parentItem && parentItem.objectName !== "tabBarStackView") {
            parentItem = parentItem.parent
        }
        
        if (parentItem && typeof parentItem.push === "function") {
            parentItem.push(PageController.getPagePath(PageEnum.PageSettingsServerRestoreMode), {
                "backupFilePath": localPath,
                "backupFileName": fileName,
                "serverName": serverName,
                "serverIp": serverIp
            })
        } else {
            console.warn("Could not find StackView to navigate to restore mode page")
        }
    }

    // ============ Backup Controller Connections ============

    Connections {
        target: ServersBackupController

        function onBackupCreated(backupFilename) {
            // If auto-download is not enabled, show success message
            PageController.showBusyIndicator(false)
            PageController.showNotificationMessage(qsTr("Backup created successfully: %1").arg(backupFilename))
        }

        function onBackupDownloaded(localPath) {
            PageController.showBusyIndicator(false)
            console.log("Backup downloaded to:", localPath)
            PageController.showNotificationMessage(qsTr("Backup downloaded successfully!\n\nSaved to:\n%1").arg(localPath))
        }

        function onBackupRestored() {
            PageController.showBusyIndicator(false)
            PageController.showNotificationMessage(qsTr("Backup restored successfully! Containers are restarting..."))
        }

        function onProgressChanged(percent, message) {
            console.log("Backup progress:", percent, "%", message)
        }

        function onErrorOccurred(errorMessage, errorCode) {
            PageController.showBusyIndicator(false)
            PageController.showErrorMessage(qsTr("Backup error: %1").arg(errorMessage))
        }
    }
}
