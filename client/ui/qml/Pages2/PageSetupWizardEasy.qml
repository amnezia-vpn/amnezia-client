import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ContainerProps 1.0
import ProtocolProps 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Config"

PageType {
    id: root
    objectName: "pageSetupWizardEasy"

    property bool isEasySetup: true
    property bool isRestoreFromBackup: false
    property string backupFilePath: ""
    property string restoreHostname: ""
    property string restoreUsername: ""
    property string restoreSecretData: ""
    property bool waitingForServerToAdd: false
    
    // Для установки контейнеров из backup
    property var containersToInstall: []
    property int currentContainerIndex: 0
    property bool isInstallingContainers: false
    
    // Connections для отслеживания добавления сервера
    Connections {
        target: InstallController
        
        function onInstallServerFinished(finishedMessage) {
            if (root.waitingForServerToAdd && root.isRestoreFromBackup && root.backupFilePath.length > 0) {
                console.log("Server added successfully, now installing containers from backup...")
                root.waitingForServerToAdd = false
                
                // Сервер уже создан, устанавливаем флаг в false
                InstallController.setShouldCreateServer(false)
                
                // Начинаем установку контейнеров
                root.isInstallingContainers = true
                installNextContainer()
            }
        }
        
        function onInstallContainerFinished(finishedMessage, isServiceInstall) {
            if (root.isInstallingContainers) {
                console.log("Container installed:", finishedMessage)
                
                // Переходим к следующему контейнеру
                root.currentContainerIndex++
                
                if (root.currentContainerIndex < root.containersToInstall.length) {
                    // Устанавливаем следующий контейнер
                    installNextContainer()
                } else {
                    // Все контейнеры установлены, теперь делаем restore
                    console.log("All containers installed, starting restore...")
                    root.isInstallingContainers = false
                    
                    // ВАЖНО: Выключаем busy indicator перед переходом
                    PageController.showBusyIndicator(false)
                    
                    // Запускаем переход на страницу выбора режима restore
                    navigationTimer.start()
                }
            }
        }
    }
    
    // Функция для установки следующего контейнера из списка
    function installNextContainer() {
        if (root.currentContainerIndex >= root.containersToInstall.length) {
            return
        }
        
        var containerName = root.containersToInstall[root.currentContainerIndex]
        console.log("Installing container:", containerName, "(", root.currentContainerIndex + 1, "/", root.containersToInstall.length, ")")
        
        // Конвертируем имя контейнера в DockerContainer enum
        var dockerContainer = ContainerProps.containerFromString(containerName)
        
        if (dockerContainer === 0) { // None
            console.log("Unknown container:", containerName, "skipping...")
            root.currentContainerIndex++
            installNextContainer()
            return
        }
        
        // Получаем default настройки для контейнера
        var defaultProtocol = ContainerProps.defaultProtocol(dockerContainer)
        var defaultPort = ProtocolProps.getPortForInstall(defaultProtocol)
        var defaultTransport = ProtocolProps.defaultTransportProto(defaultProtocol)
        
        // Показываем индикатор загрузки с сообщением
        PageController.showBusyIndicator(true)
        PageController.showNotificationMessage(qsTr("Installing %1 (%2/%3)...")
            .arg(containerName)
            .arg(root.currentContainerIndex + 1)
            .arg(root.containersToInstall.length))
        
        // Убеждаемся что credentials установлены
        console.log("Setting credentials for container installation...")
        InstallController.setProcessedServerCredentials(root.restoreHostname, root.restoreUsername, root.restoreSecretData)
        
        // Устанавливаем индекс сервера
        var serverIdx = ServersModel.getServersCount() - 1
        ServersModel.processedIndex = serverIdx
        
        // Устанавливаем контейнер
        console.log("Calling InstallController.install for docker container:", dockerContainer)
        ContainersModel.setProcessedContainerIndex(dockerContainer)
        InstallController.install(dockerContainer, defaultPort, defaultTransport)
    }
    
    // Таймер для перехода на страницу выбора режима после выбора файла
    Timer {
        id: navigationTimer
        interval: 500
        repeat: false
        onTriggered: {
            if (root.backupFilePath.length > 0 && root.isRestoreFromBackup) {
                console.log("Navigation timer triggered, going to restore mode page")
                console.log("Credentials available:", root.restoreHostname, root.restoreUsername, root.restoreSecretData.length > 0 ? "***" : "EMPTY")
                
                // Получаем имя файла
                var fileName = SystemController.getFileNameFromPath(root.backupFilePath)
                if (!fileName || fileName === undefined || fileName.length === 0) {
                    var fallbackName = root.backupFilePath.split('/').pop()
                    fileName = (fallbackName && fallbackName.length > 0) ? fallbackName : qsTr("backup.tgz")
                }
                fileName = String(fileName)
                
                // Извлекаем IP адрес из имени файла
                var serverIp = ""
                var ipMatch = fileName.match(/^([\d_]+)\s*-/)
                if (ipMatch && ipMatch.length > 1) {
                    serverIp = ipMatch[1].replace(/_/g, ".")
                }
                if (!serverIp || serverIp.length === 0) {
                    serverIp = root.restoreHostname
                }
                
                var serverName = root.restoreHostname
                if (!serverName || serverName.length === 0) {
                    serverName = qsTr("RestoredServer")
                }
                
                // Переходим на страницу установки
                PageController.goToPage(PageEnum.PageSetupWizardInstalling)
                
                // Сразу ищем StackView и переходим на страницу восстановления
                // Сервер уже добавлен, так как мы ждали onInstallServerFinished
                Qt.callLater(function() {
                    var pagePath = "qrc:/ui/qml/Pages2/PageSettingsServerRestoreMode.qml"
                    
                    // Находим главное окно приложения
                    var item = root
                    while (item.parent) {
                        item = item.parent
                    }
                    
                    // Находим StackView рекурсивно
                    function findStackView(obj) {
                        if (!obj) return null
                        
                        // Проверяем, является ли объект StackView
                        if (obj.toString().indexOf("StackView") !== -1 || typeof obj.push === "function") {
                            return obj
                        }
                        
                        // Проверяем children
                        if (obj.children) {
                            for (var i = 0; i < obj.children.length; i++) {
                                var result = findStackView(obj.children[i])
                                if (result) return result
                            }
                        }
                        
                        // Проверяем contentItem
                        if (obj.contentItem) {
                            return findStackView(obj.contentItem)
                        }
                        
                        return null
                    }
                    
                    var stackView = findStackView(item)
                    if (stackView) {
                        console.log("Found StackView, pushing restore mode page")
                        stackView.push(pagePath, {
                            "backupFilePath": root.backupFilePath,
                            "backupFileName": fileName,
                            "serverName": "", // Будет получено из ServersModel
                            "serverIp": serverIp,
                            "isFromSetupWizard": true,
                            "wizardHostname": root.restoreHostname,
                            "wizardUsername": root.restoreUsername,
                            "wizardSecretData": root.restoreSecretData
                        })
                    } else {
                        console.error("Could not find StackView")
                    }
                })
            }
        }
    }

    SortFilterProxyModel {
        id: proxyContainersModel
        
        sourceModel: ContainersModel
        filters: [
            ValueFilter {
                roleName: "isEasySetupContainer"
                value: true
            }
        ]
        sorters: RoleSorter {
            roleName: "easySetupOrder"
            sortOrder: Qt.DescendingOrder
        }
    }

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20 + PageController.safeAreaTopMargin

        onActiveFocusChanged: {
            if(backButton.enabled && backButton.activeFocus) {
                listView.positionViewAtBeginning()
            }
        }
    }

    ButtonGroup {
        id: buttonGroup
    }

    ListViewType {
        id: listView

        property int dockerContainer
        property int containerDefaultPort
        property int containerDefaultTransportProto

        anchors.top: backButton.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        spacing: 16

        header: ColumnLayout {
            width: listView.width

            spacing: 16

            BaseHeaderType {
                id: header

                Layout.fillWidth: true
                Layout.rightMargin: 16
                Layout.leftMargin: 16
                Layout.bottomMargin: 16

                headerTextMaximumLineCount: 10

                headerText: qsTr("Choose Installation Type")
            }
        }

        model: proxyContainersModel
        currentIndex: 0

        delegate: ColumnLayout {
            width: listView.width

            CardType {
                id: card

                Layout.fillWidth: true
                Layout.rightMargin: 16
                Layout.leftMargin: 16
                Layout.bottomMargin: 16

                headerText: easySetupHeader
                bodyText: easySetupDescription

                ButtonGroup.group: buttonGroup

                onClicked: function() {
                    isEasySetup = true
                    checked = true
                    var defaultContainerProto =  ContainerProps.defaultProtocol(dockerContainer)

                    listView.dockerContainer = dockerContainer
                    listView.containerDefaultPort = InstallController.getPortForInstall(defaultContainerProto)
                    listView.containerDefaultTransportProto = InstallController.defaultTransportProto(defaultContainerProto)
                }

                Keys.onReturnPressed: this.clicked()
                Keys.onEnterPressed: this.clicked()
            }
        }

        footer: ColumnLayout {
            width: listView.width
            spacing: 16

            DividerType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
            }

            CardType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("Manual")
                bodyText: qsTr("Choose a VPN protocol")

                ButtonGroup.group: buttonGroup

                onClicked: function() {
                    isEasySetup = false
                    checked = true
                }

                Keys.onEnterPressed: this.clicked()
                Keys.onReturnPressed: this.clicked()
            }

            DividerType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
            }

            CardType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("Restore from backup")
                bodyText: qsTr("Restoration of VPN protocols, services, all server settings and users")

                ButtonGroup.group: buttonGroup

                onClicked: function() {
                    console.log("=== Restore from backup clicked ===")
                    
                    // СНАЧАЛА выбираем файл
                    var filter = GC.isMobile() ? "*.gz *.tgz *.tar.gz" : "Backup files (*.tar.gz *.backup *.tgz *.gz)"
                    var localPath = SystemController.getFileName(
                        qsTr("Select Backup to Restore"), 
                        filter,
                        "",
                        false,
                        ""
                    )
                    
                    console.log("Selected file path:", localPath)
                    
                    if (!localPath || localPath.length === 0) {
                        console.log("No file selected")
                        return
                    }
                    
                    // Сохраняем путь к backup файлу
                    root.backupFilePath = localPath
                    root.isRestoreFromBackup = true
                    
                    // Сканируем backup для определения контейнеров
                    console.log("Scanning backup for containers...")
                    var foundContainers = ServersBackupController.scanBackupForContainers(localPath)
                    console.log("Found containers:", foundContainers)
                    
                    if (foundContainers.length === 0) {
                        PageController.showErrorMessage(qsTr("No containers found in backup file"))
                        root.isRestoreFromBackup = false
                        return
                    }
                    
                    root.containersToInstall = foundContainers
                    root.currentContainerIndex = 0
                    
                    // Получаем credentials из PageSetupWizardCredentials через поиск в StackView
                    var credentialsPage = null
                    var item = root
                    
                    // Ищем StackView
                    while (item && !item.hasOwnProperty("depth")) {
                        item = item.parent
                    }
                    
                    // Если нашли StackView, ищем PageSetupWizardCredentials в его истории
                    if (item && item.depth > 0) {
                        for (var i = 0; i < item.depth; i++) {
                            var page = item.get(i)
                            if (page && page.hasOwnProperty("savedHostname")) {
                                credentialsPage = page
                                break
                            }
                        }
                    }
                    
                    if (credentialsPage && credentialsPage.savedHostname.length > 0) {
                        root.restoreHostname = credentialsPage.savedHostname
                        root.restoreUsername = credentialsPage.savedUsername
                        root.restoreSecretData = credentialsPage.savedSecretData
                        console.log("Got credentials from PageSetupWizardCredentials:", root.restoreHostname, root.restoreUsername)
                        
                        // ТЕПЕРЬ добавляем пустой сервер с этими credentials
                        InstallController.setShouldCreateServer(true)
                        InstallController.setProcessedServerCredentials(root.restoreHostname, root.restoreUsername, root.restoreSecretData)
                        
                        // Устанавливаем флаг ожидания
                        root.waitingForServerToAdd = true
                        
                        console.log("Backup file selected, adding server...")
                        // Добавляем сервер (асинхронно)
                        InstallController.addEmptyServer()
                        
                        // Дальнейшее выполнение произойдет в onInstallServerFinished
                    } else {
                        console.log("WARNING: No credentials found")
                        return
                    }
                }

                Keys.onEnterPressed: this.clicked()
                Keys.onReturnPressed: this.clicked()
            }

            BasicButtonType {
                id: continueButton

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Continue")

                clickedFunc: function() {
                    if (root.isEasySetup) {
                        ServersUiController.processedContainerIndex = listView.dockerContainer
                        PageController.goToPage(PageEnum.PageSetupWizardInstalling)
                        InstallController.install(listView.dockerContainer,
                                                      listView.containerDefaultPort,
                                                      listView.containerDefaultTransportProto,
                                                      ServersUiController.processedServerId)
                    } else {
                        PageController.goToPage(PageEnum.PageSetupWizardProtocols)
                    }
                }
            }

            BasicButtonType {
                id: setupLaterButton

                Layout.fillWidth: true
                Layout.topMargin: 8
                Layout.bottomMargin: 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                defaultColor: AmneziaStyle.color.transparent
                hoveredColor: AmneziaStyle.color.translucentWhite
                pressedColor: AmneziaStyle.color.sheerWhite
                disabledColor: AmneziaStyle.color.mutedGray
                textColor: AmneziaStyle.color.paleGray
                borderWidth: 1

                visible: {
                    if (PageController.isTriggeredByConnectButton()) {
                        PageController.setTriggeredByConnectButton(false)
                        return false
                    }

                    return  true
                }

                text: qsTr("Skip setup")

                clickedFunc: function() {
                    PageController.goToPage(PageEnum.PageSetupWizardInstalling)
                    InstallController.addEmptyServer()
                }
            }
        }

        Component.onCompleted: {
            var item = listView.itemAtIndex(listView.currentIndex)
            if (item !== null) {
                var button = item.children[0]
                button.checked = true
                button.clicked()
            }
        }
    }
}

