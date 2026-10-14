import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"

PageType {
    id: root

    FlickableType {
        id: flickable

        anchors.top: parent.top
        anchors.bottom: bottomBar.top

        contentHeight: content.implicitHeight + 16

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 0

            NavigationBarType {
                Layout.fillWidth: true

                title: qsTr("Backup")
                description: qsTr("Local copy of VPN protocols, services, all server settings and users.")
            }

            LinkButtonType {
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("More about backups")

                onClicked: {
                    // TODO: Open help page or show more info
                }
            }
        }
    }

    BottomBarType {
        id: bottomBar

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        filled: true

        ButtonType {
            Layout.fillWidth: true

            text: qsTr("Create backup")

            onClicked: createBackup(true)
        }

        ButtonType {
            Layout.fillWidth: true

            variant: ButtonType.Secondary

            text: qsTr("Restore from backup")

            onClicked: restoreBackup()
        }
    }

    function createBackup(shouldDownload) {
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
            ServersBackupController.createBackupWithDownload(downloadAfterCreate, true)
        }
        var noButtonFunction = function() {}

        showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
    }

    function restoreBackup() {
        var filter = GC.isMobile() ? "*.gz *.tgz *.tar.gz" : "Backup files (*.tar.gz *.backup *.tgz *.gz)"
        var localPath = SystemController.getFileName(qsTr("Select Backup to Restore"), filter, "", false, "")

        if (!localPath || localPath.length === 0) {
            return
        }

        var fileInfo = ServersBackupController.getBackupFileInfo(localPath)
        var fileName = fileInfo.fileName || "backup.tgz"
        var serverIp = fileInfo.serverIp || ""

        if (!serverIp || serverIp.length === 0) {
            serverIp = ServersUiController.serverHostName(ServersUiController.processedServerId) || ""
        }

        var serverName = ServersUiController.serverName(ServersUiController.processedServerId) || qsTr("Server")

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

    Connections {
        target: ServersBackupController

        function onBackupCreated(backupFilename) {
            PageController.showBusyIndicator(false)
            PageController.showNotificationMessage(qsTr("Backup created successfully: %1").arg(backupFilename))
        }

        function onBackupDownloaded(localPath) {
            PageController.showBusyIndicator(false)
            PageController.showNotificationMessage(qsTr("Backup downloaded successfully!\n\nSaved to:\n%1").arg(localPath))
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
