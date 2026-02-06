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
    
    // For installing containers from backup
    property var containersToInstall: []
    property int currentContainerIndex: 0
    property bool isInstallingContainers: false
    
    // Connections for ServersBackupController
    Connections {
        target: ServersBackupController
        
        function onReadyForRestore(backupFilePath, hostname, username, secretData, serverIp, fileName) {
            console.log("onReadyForRestore received from C++")
            console.log("  backupFilePath:", backupFilePath)
            console.log("  hostname:", hostname)
            console.log("  serverIp:", serverIp)
            console.log("  fileName:", fileName)
            
            // Scan backup to determine containers (C++ already did this, but needed for QML)
            var foundContainers = ServersBackupController.scanBackupForContainers(backupFilePath)
            console.log("Found containers:", foundContainers)
            
            if (foundContainers.length === 0) {
                PageController.showErrorMessage(qsTr("No containers found in backup file"))
                root.isRestoreFromBackup = false
                return
            }
            
            root.containersToInstall = foundContainers
            root.currentContainerIndex = 0
            
            // Now add empty server with these credentials
            InstallController.setShouldCreateServer(true)
            InstallController.setProcessedServerCredentials(hostname, username, secretData)
            
            // Set waiting flag
            root.waitingForServerToAdd = true
            
            console.log("Backup scanned, adding server...")
            // Add server (asynchronously)
            InstallController.addEmptyServer()
            
            // Further execution will happen in onInstallServerFinished
        }
    }
    
    // Connections for tracking server addition
    Connections {
        target: InstallController
        
        function onInstallServerFinished(finishedMessage) {
            if (root.waitingForServerToAdd && root.isRestoreFromBackup && root.backupFilePath.length > 0) {
                console.log("Server added successfully, now installing containers from backup...")
                root.waitingForServerToAdd = false
                
                // Server already created, set flag to false
                InstallController.setShouldCreateServer(false)
                
                // Start installing containers
                root.isInstallingContainers = true
                installNextContainer()
            }
        }
        
        function onInstallContainerFinished(finishedMessage, isServiceInstall) {
            if (root.isInstallingContainers) {
                console.log("Container installed:", finishedMessage)
                
                // Move to next container
                root.currentContainerIndex++
                
                if (root.currentContainerIndex < root.containersToInstall.length) {
                    // Install next container
                    installNextContainer()
                } else {
                    // All containers installed, now do restore
                    console.log("All containers installed, starting restore...")
                    root.isInstallingContainers = false
                    
                    // IMPORTANT: Turn off busy indicator before navigation
                    PageController.showBusyIndicator(false)
                    
                    // Start navigation to restore mode selection page
                    navigationTimer.start()
                }
            }
        }
    }
    
    // Function to install next container from list
    function installNextContainer() {
        if (root.currentContainerIndex >= root.containersToInstall.length) {
            return
        }
        
        var containerName = root.containersToInstall[root.currentContainerIndex]
        console.log("Installing container:", containerName, "(", root.currentContainerIndex + 1, "/", root.containersToInstall.length, ")")
        
        // Convert container name to DockerContainer enum
        var dockerContainer = ContainerProps.containerFromString(containerName)
        
        if (dockerContainer === 0) { // None
            console.log("Unknown container:", containerName, "skipping...")
            root.currentContainerIndex++
            installNextContainer()
            return
        }
        
        // Get default settings for container
        var defaultProtocol = ContainerProps.defaultProtocol(dockerContainer)
        var defaultPort = ProtocolProps.getPortForInstall(defaultProtocol)
        var defaultTransport = ProtocolProps.defaultTransportProto(defaultProtocol)
        
        // Show loading indicator with message
        PageController.showBusyIndicator(true)
        PageController.showNotificationMessage(qsTr("Installing %1 (%2/%3)...")
            .arg(containerName)
            .arg(root.currentContainerIndex + 1)
            .arg(root.containersToInstall.length))
        
        // Ensure credentials are set
        console.log("Setting credentials for container installation...")
        InstallController.setProcessedServerCredentials(root.restoreHostname, root.restoreUsername, root.restoreSecretData)
        
        // Set server index
        var serverIdx = ServersModel.getServersCount() - 1
        ServersModel.processedIndex = serverIdx
        
        // Install container
        console.log("Calling InstallController.install for docker container:", dockerContainer)
        ContainersModel.setProcessedContainerIndex(dockerContainer)
        InstallController.install(dockerContainer, defaultPort, defaultTransport)
    }
    
    // Timer for navigating to restore mode selection page after file selection
    Timer {
        id: navigationTimer
        interval: 500
        repeat: false
        onTriggered: {
            if (root.backupFilePath.length > 0 && root.isRestoreFromBackup) {
                console.log("Navigation timer triggered, going to restore mode page")
                console.log("Credentials available:", root.restoreHostname, root.restoreUsername, root.restoreSecretData.length > 0 ? "***" : "EMPTY")
                
                // Get filename
                var fileName = SystemController.getFileNameFromPath(root.backupFilePath)
                if (!fileName || fileName === undefined || fileName.length === 0) {
                    var fallbackName = root.backupFilePath.split('/').pop()
                    fileName = (fallbackName && fallbackName.length > 0) ? fallbackName : qsTr("backup.tgz")
                }
                fileName = String(fileName)
                
                // Extract IP address from filename
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
                
                // Navigate to installation page
                PageController.goToPage(PageEnum.PageSetupWizardInstalling)
                
                // Immediately find StackView and navigate to restore page
                // Server already added, as we waited for onInstallServerFinished
                Qt.callLater(function() {
                    var pagePath = "qrc:/ui/qml/Pages2/PageSettingsServerRestoreMode.qml"
                    
                    // Find main application window
                    var item = root
                    while (item.parent) {
                        item = item.parent
                    }
                    
                    // Find StackView recursively
                    function findStackView(obj) {
                        if (!obj) return null
                        
                        // Check if object is StackView
                        if (obj.toString().indexOf("StackView") !== -1 || typeof obj.push === "function") {
                            return obj
                        }
                        
                        // Check children
                        if (obj.children) {
                            for (var i = 0; i < obj.children.length; i++) {
                                var result = findStackView(obj.children[i])
                                if (result) return result
                            }
                        }
                        
                        // Check contentItem
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
                            "serverName": "", // Will be obtained from ServersModel
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
                    
                    // Save backup file path
                    root.backupFilePath = localPath
                    root.isRestoreFromBackup = true
                    
                    // Get credentials from PageSetupWizardCredentials via StackView search
                    var credentialsPage = null
                    var item = root
                    
                    // Find StackView
                    while (item && !item.hasOwnProperty("depth")) {
                        item = item.parent
                    }
                    
                    // If found StackView, search for PageSetupWizardCredentials in its history
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
                        
                        // Call C++ method to prepare restore
                        // It will scan backup and send readyForRestore signal
                        ServersBackupController.prepareRestoreFromBackup(localPath, root.restoreHostname, root.restoreUsername, root.restoreSecretData)
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

