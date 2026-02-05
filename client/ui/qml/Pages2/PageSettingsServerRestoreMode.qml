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

    property string backupFilePath: ""
    property string backupFileName: ""
    property string serverName: ""
    property string serverIp: ""
    property bool isFromSetupWizard: false
    
    // Credentials для setup wizard (когда сервер еще не добавлен в ServersModel)
    property string wizardHostname: ""
    property string wizardUsername: ""
    property string wizardSecretData: ""

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
                
                headerText: qsTr("Restore from backup")
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                
                text: {
                    // Показываем только имя файла и IP адрес, без имени сервера
                    if (serverIp && serverIp.length > 0) {
                        return qsTr("%1 on %2").arg(backupFileName).arg(serverIp)
                    }
                    return backupFileName
                }
                color: AmneziaStyle.color.mutedGray
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 16
                spacing: 0

                LabelWithButtonType {
                    Layout.fillWidth: true

                    text: qsTr("Add data from backup")
                    descriptionText: qsTr("If the same protocols are already installed on the server, they will be updated. Created users and access will be saved")
                    rightImageSource: "qrc:/images/controls/chevron-right.svg"

                    clickedFunction: function() {
                        startRestore(false) // false = режим добавления
                    }
                }

                DividerType {}

                LabelWithButtonType {
                    Layout.fillWidth: true

                    text: qsTr("Replace")
                    descriptionText: qsTr("All installed protocols, users and their access will not be saved")
                    rightImageSource: "qrc:/images/controls/chevron-right.svg"
                    textColor: AmneziaStyle.color.vibrantRed

                    clickedFunction: function() {
                        startRestore(true) // true = режим замены
                    }
                }
            }
        }
    }

    property bool restoreReplaceMode: false

    function startRestore(replaceMode) {
        restoreReplaceMode = replaceMode
        
        // Если это setup wizard с wizard credentials, используем их напрямую
        if (isFromSetupWizard && wizardHostname.length > 0) {
            console.log("Setup wizard mode, using uploadBackupWithStrings")
            PageController.showBusyIndicator(true)
            ServersBackupController.uploadBackupWithStrings(
                wizardHostname,
                wizardUsername,
                wizardSecretData,
                backupFilePath,
                replaceMode
            )
        } else {
            // Обычный режим - берем из ServersModel
            PageController.showBusyIndicator(true)
            var credentials = getServerCredentials()
            ServersBackupController.uploadBackup(credentials, backupFilePath, replaceMode)
        }
    }

    function getServerCredentials() {
        // Если это setup wizard, используем переданные credentials
        if (isFromSetupWizard && wizardHostname.length > 0) {
            console.log("Using wizard credentials:", wizardHostname, wizardUsername)
            // Устанавливаем credentials в InstallController
            InstallController.setProcessedServerCredentials(wizardHostname, wizardUsername, wizardSecretData)
            // Получаем их обратно как C++ объект через ServersModel
            // Сначала проверяем, есть ли сервер в модели
            if (ServersModel.getServersCount() > 0 && ServersModel.processedIndex >= 0) {
                return ServersModel.getServerCredentials(ServersModel.processedIndex)
            }
            // Если сервера нет, создаем временный объект (это не сработает, нужен другой подход)
            console.error("Server not in model yet, cannot get credentials")
        }
        
        // Иначе берем из ServersModel
        var index = ServersModel.processedIndex
        return ServersModel.getServerCredentials(index)
    }

    property string lastUploadedBackupFilename: ""

    Connections {
        target: ServersBackupController

        function onBackupUploaded(serverPath) {
            PageController.showNotificationMessage(qsTr("Backup uploaded. Restoring configuration..."))
            
            var backupFilename = serverPath.split('/').pop()
            lastUploadedBackupFilename = backupFilename
            
            var credentials = getServerCredentials()
            ServersBackupController.restoreBackup(credentials, backupFilename, [], restoreReplaceMode)
        }

        function onBackupRestored() {

            console.log("  onBackupRestored, isFromSetupWizard:", isFromSetupWizard)
            
            PageController.showBusyIndicator(false)
            
            // Для setup wizard устанавливаем default container и сервер
            if (isFromSetupWizard && ServersModel.getServersCount() > 0) {
                var serverIdx = ServersModel.getServersCount() - 1
                console.log("  Setting server as default:", serverIdx)
                ServersModel.setDefaultServerIndex(serverIdx)
                ServersModel.processedIndex = serverIdx
                
                // Запускаем timer для установки default container
                // Контейнеры уже установлены через InstallController, просто ждем обновления модели
                setDefaultContainerTimer.start()
            } else {
                // Для обычного режима сразу переходим
                navigateToRestoredPage()
            }
        }

        function onErrorOccurred(errorMessage, errorCode) {
            PageController.showBusyIndicator(false)
            PageController.showErrorMessage(qsTr("Backup restore error: %1").arg(errorMessage))
        }
    }
    
    // Удаляем Connections для scanServerFinished - больше не нужен
    
    function navigateToRestoredPage() {
        // Переход на страницу успешного восстановления
        // Получаем реальное имя сервера из модели
        var actualServerName = serverName
        if (root.isFromSetupWizard && ServersModel.getServersCount() > 0) {
            var serverIdx = ServersModel.getServersCount() - 1
            var oldProcessedIndex = ServersModel.processedIndex
            ServersModel.processedIndex = serverIdx
            actualServerName = ServersModel.getProcessedServerData("name") || qsTr("Server")
            ServersModel.processedIndex = oldProcessedIndex
        } else if (!serverName || serverName.length === 0) {
            // Если имя не передано, получаем из processedIndex
            actualServerName = ServersModel.getProcessedServerData("name") || qsTr("Server")
        }
        
        var parentItem = root.parent
        
        // Для setup wizard используем обычный StackView
        if (root.isFromSetupWizard) {
            while (parentItem && typeof parentItem.push !== "function") {
                parentItem = parentItem.parent
            }
            if (parentItem && typeof parentItem.push === "function") {
                parentItem.push(PageController.getPagePath(PageEnum.PageSettingsServerBackupRestored), {
                    "backupFileName": backupFileName,
                    "serverName": actualServerName,
                    "serverIp": serverIp,
                    "isFromSetupWizard": true
                })
            }
        } else {
            // Для меню управления ищем tabBarStackView
            while (parentItem && parentItem.objectName !== "tabBarStackView") {
                parentItem = parentItem.parent
            }
            if (parentItem && typeof parentItem.push === "function") {
                parentItem.push(PageController.getPagePath(PageEnum.PageSettingsServerBackupRestored), {
                    "backupFileName": backupFileName,
                    "serverName": actualServerName,
                    "serverIp": serverIp,
                    "isFromSetupWizard": false
                })
            } else {
                console.warn("Could not find StackView to navigate to restored page")
            }
        }
    }
    
    property int containerRetryCount: 0
    property int maxContainerRetries: 5
    
    Timer {
        id: setDefaultContainerTimer
        interval: 1000
        repeat: false
        
        onTriggered: {
            console.log("Timer: Searching for installed containers (attempt", containerRetryCount + 1, "/", maxContainerRetries, ")")
            var serverIdx = ServersModel.getServersCount() - 1
            
            // Ищем первый установленный контейнер через DefaultServerContainersModel
            console.log("  Total rows:", DefaultServerContainersModel.rowCount())
            var foundInstalled = false
            for (var i = 0; i < DefaultServerContainersModel.rowCount(); i++) {
                var isInstalled = DefaultServerContainersModel.data(DefaultServerContainersModel.index(i, 0), 0x0012) // IsInstalledRole
                if (isInstalled) {
                    console.log("  Setting default container:", i, "for server:", serverIdx)
                    ServersModel.setDefaultContainer(serverIdx, i)
                    foundInstalled = true
                    containerRetryCount = 0 // Reset counter
                    
                    // Выключаем индикатор загрузки и переходим на страницу результата
                    PageController.showBusyIndicator(false)
                    navigateToRestoredPage()
                    return
                }
            }
            
            containerRetryCount++
            if (containerRetryCount < maxContainerRetries) {
                console.log("  No installed containers found yet, will retry...")
                setDefaultContainerTimer.start()
            } else {
                console.log("  Max retries reached, stopping search")
                containerRetryCount = 0 // Reset for next time
                
                // Все равно переходим на страницу результата
                PageController.showBusyIndicator(false)
                navigateToRestoredPage()
            }
        }
    }
}
