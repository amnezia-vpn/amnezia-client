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

    // ============ Backup Functions ============

    function getServerCredentials() {
        var index = ServersModel.processedIndex
        return ServersModel.getServerCredentials(index)
    }

    property bool downloadAfterCreate: false

    function createBackup(shouldDownload) {
        // По умолчанию shouldDownload = true, если не указано
        downloadAfterCreate = (shouldDownload !== undefined) ? shouldDownload : true
        
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
            
            var credentials = getServerCredentials()
            // Всегда создаем backup всех контейнеров
            ServersBackupController.createBackup(credentials)
        }
        var noButtonFunction = function() {}

        showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
    }

    property string selectedBackupForRestore: ""

    function restoreBackup() {
        // Для мобильных устройств используем все возможные расширения backup файлов
        // Android преобразует расширения в MIME типы автоматически
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
        
        selectedBackupForRestore = localPath
        
        // Открываем страницу выбора режима восстановления
        var parentItem = root.parent
        while (parentItem && parentItem.objectName !== "tabBarStackView") {
            parentItem = parentItem.parent
        }
        if (parentItem && typeof parentItem.push === "function") {
            // Используем SystemController для получения имени файла из пути или URI
            // Это правильно обработает Android URI через ContentResolver
            var fileName = SystemController.getFileNameFromPath(localPath)
            
            // Если имя файла пустое или undefined, используем fallback
            if (!fileName || fileName === undefined || fileName.length === 0) {
                var fallbackName = localPath.split('/').pop()
                fileName = (fallbackName && fallbackName.length > 0) ? fallbackName : qsTr("backup.tgz")
            }
            
            // Убеждаемся, что fileName - это строка
            fileName = String(fileName)
            
            // Извлекаем IP адрес из имени файла (формат: IP_ADDRESS - DD-MM-YYYY_HH-MM-SS.tgz)
            var serverIp = ""
            var ipMatch = fileName.match(/^([\d_]+)\s*-/)
            if (ipMatch && ipMatch.length > 1) {
                // Заменяем подчеркивания на точки для отображения IP адреса
                serverIp = ipMatch[1].replace(/_/g, ".")
            }
            
            // Если не удалось извлечь IP из имени файла, используем IP из credentials
            if (!serverIp || serverIp.length === 0) {
                var credentials = getServerCredentials()
                serverIp = credentials.hostName || ""
            }
            
            var serverName = ServersModel.getProcessedServerData("name") || qsTr("Server")
            
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

    property string lastCreatedBackupFilename: ""
    property string lastUploadedBackupFilename: ""

    Connections {
        target: ServersBackupController

        function onBackupCreated(backupFilename) {
            lastCreatedBackupFilename = backupFilename
            
            if (downloadAfterCreate) {
                var credentials = getServerCredentials()
                var localPath = backupFilename
                PageController.showNotificationMessage(qsTr("Backup created. Downloading to device..."))
                ServersBackupController.downloadBackup(credentials, backupFilename, localPath)
                downloadAfterCreate = false
            } else {
                PageController.showBusyIndicator(false)
                PageController.showNotificationMessage(qsTr("Backup created successfully: %1").arg(backupFilename))
            }
        }

        function onBackupDownloaded(localPath) {
            PageController.showBusyIndicator(false)
            console.log("Backup downloaded to:", localPath)
            
            if (lastCreatedBackupFilename && lastCreatedBackupFilename.length > 0) {
                var credentials = getServerCredentials()
                ServersBackupController.deleteBackup(credentials, lastCreatedBackupFilename)
                console.log("Deleting backup from server:", lastCreatedBackupFilename)
            }
            
            PageController.showNotificationMessage(qsTr("Backup downloaded successfully!\n\nSaved to:\n%1").arg(localPath))
        }

        function onBackupUploaded(serverPath) {
            // Этот обработчик больше не используется здесь, так как восстановление
            // теперь происходит через PageSettingsServerRestoreMode
            // Оставляем для совместимости, но не выполняем действий
        }

        function onBackupRestored() {
            PageController.showBusyIndicator(false)
            
            selectedBackupForRestore = ""
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
