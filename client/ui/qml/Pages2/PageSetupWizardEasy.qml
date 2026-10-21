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

    property bool isEasySetup: true

    property int dockerContainer
    property int containerDefaultPort
    property int containerDefaultTransportProto

    property bool isRestoreFromBackup: false
    property string backupFilePath: ""
    property bool waitingForServerToAdd: false

    property var containersToInstall: []
    property int currentContainerIndex: 0
    property bool isInstallingContainers: false

    Connections {
        target: InstallController

        function onInstallServerFinished(finishedMessage) {
            if (root.waitingForServerToAdd && root.isRestoreFromBackup && root.backupFilePath.length > 0) {
                root.waitingForServerToAdd = false
                ServersUiController.setProcessedServerId(ServersUiController.getServerId(ServersUiController.getServersCount() - 1))
                root.isInstallingContainers = true
                installNextContainer()
            }
        }

        function onInstallContainerFinished(finishedMessage, isServiceInstall) {
            if (root.isInstallingContainers) {
                root.currentContainerIndex++

                if (root.currentContainerIndex < root.containersToInstall.length) {
                    installNextContainer()
                } else {
                    root.isInstallingContainers = false
                    PageController.showBusyIndicator(false)
                    navigationTimer.start()
                }
            }
        }
    }

    function installNextContainer() {
        if (root.currentContainerIndex >= root.containersToInstall.length) {
            return
        }

        var containerName = root.containersToInstall[root.currentContainerIndex]
        var dockerContainer = ContainerProps.containerFromString(containerName)

        if (dockerContainer === 0) {
            console.log("Unknown container:", containerName, "skipping...")
            root.currentContainerIndex++
            installNextContainer()
            return
        }

        var defaultProtocol = ContainerProps.defaultProtocol(dockerContainer)
        var defaultPort = InstallController.getPortForInstall(defaultProtocol)
        var defaultTransport = InstallController.defaultTransportProto(defaultProtocol)

        var serverId = ServersUiController.processedServerId

        PageController.showBusyIndicator(true)
        PageController.showNotificationMessage(qsTr("Installing %1 (%2/%3)...")
            .arg(containerName)
            .arg(root.currentContainerIndex + 1)
            .arg(root.containersToInstall.length))

        console.log("Installing container:", containerName, "serverId:", serverId)
        ServersUiController.processedContainerIndex = dockerContainer
        InstallController.install(dockerContainer, defaultPort, defaultTransport, serverId)
    }

    Timer {
        id: navigationTimer
        interval: 500
        repeat: false
        onTriggered: {
            if (root.backupFilePath.length > 0 && root.isRestoreFromBackup) {
                var fileName = SystemController.getFileNameFromPath(root.backupFilePath)
                if (!fileName || fileName === undefined || fileName.length === 0) {
                    var fallbackName = root.backupFilePath.split('/').pop()
                    fileName = (fallbackName && fallbackName.length > 0) ? fallbackName : qsTr("backup.tgz")
                }
                fileName = String(fileName)

                var serverIp = ""
                var ipMatch = fileName.match(/^([\d_]+)\s*-/)
                if (ipMatch && ipMatch.length > 1) {
                    serverIp = ipMatch[1].replace(/_/g, ".")
                }
                if (!serverIp || serverIp.length === 0) {
                    serverIp = ServersUiController.serverHostName(ServersUiController.processedServerId)
                }

                PageController.goToPage(PageEnum.PageSetupWizardInstalling)

                Qt.callLater(function() {
                    var pagePath = "qrc:/ui/qml/Pages2/PageSettingsServerRestoreMode.qml"

                    // Walk up to the containing StackView instead of a recursive downward search,
                    // which overflows the JS stack on iOS with a large component tree.
                    var stackView = root.parent
                    while (stackView) {
                        if (typeof stackView.push === "function" && stackView.hasOwnProperty("depth")) {
                            break
                        }
                        stackView = stackView.parent
                    }

                    if (stackView) {
                        stackView.push(pagePath, {
                            "backupFilePath": root.backupFilePath,
                            "backupFileName": fileName,
                            "serverName": "",
                            "serverIp": serverIp,
                            "isFromSetupWizard": true
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

    ButtonGroup {
        id: buttonGroup
    }

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

                title: qsTr("Choose Installation Type")
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                spacing: 12

                Repeater {
                    model: proxyContainersModel

                    delegate: SelectableListItemType {
                        required property int index
                        required property int dockerContainer
                        required property string easySetupHeader
                        required property string easySetupDescription

                        Layout.fillWidth: true

                        showSeparator: index > 0

                        text: easySetupHeader
                        subtitle: easySetupDescription

                        ButtonGroup.group: buttonGroup

                        onClicked: {
                            root.isEasySetup = true
                            checked = true
                            var defaultContainerProto = ContainerProps.defaultProtocol(dockerContainer)

                            root.dockerContainer = dockerContainer
                            root.containerDefaultPort = InstallController.getPortForInstall(defaultContainerProto)
                            root.containerDefaultTransportProto = InstallController.defaultTransportProto(defaultContainerProto)
                        }

                        Component.onCompleted: {
                            if (index === 0) {
                                checked = true
                                clicked()
                            }
                        }
                    }
                }

                SelectableListItemType {
                    Layout.fillWidth: true

                    text: qsTr("Manual")
                    subtitle: qsTr("Choose a VPN protocol")

                    ButtonGroup.group: buttonGroup

                    onClicked: {
                        root.isEasySetup = false
                        checked = true
                    }
                }

                SelectableListItemType {
                    Layout.fillWidth: true

                    text: qsTr("Restore from backup")
                    subtitle: qsTr("Restoration of VPN protocols, services, all server settings and users")

                    ButtonGroup.group: buttonGroup

                    onClicked: {
                        var filter = GC.isMobile() ? "*.gz *.tgz *.tar.gz" : "Backup files (*.tar.gz *.backup *.tgz *.gz)"
                        var localPath = SystemController.getFileName(qsTr("Select Backup to Restore"), filter, "", false, "")

                        if (!localPath || localPath.length === 0) {
                            return
                        }

                        root.backupFilePath = localPath
                        root.isRestoreFromBackup = true

                        var foundContainers = BackupController.scanBackupForContainers(localPath)
                        if (foundContainers.length === 0) {
                            PageController.showErrorMessage(qsTr("No containers found in backup file"))
                            root.isRestoreFromBackup = false
                            return
                        }

                        root.containersToInstall = foundContainers
                        root.currentContainerIndex = 0

                        root.waitingForServerToAdd = true
                        InstallController.addEmptyServer()
                    }
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
            id: continueButton

            Layout.fillWidth: true

            text: qsTr("Continue")

            onClicked: {
                if (root.isEasySetup) {
                    ServersUiController.processedContainerIndex = root.dockerContainer
                    PageController.goToPage(PageEnum.PageSetupWizardInstalling)
                    InstallController.install(root.dockerContainer,
                                              root.containerDefaultPort,
                                              root.containerDefaultTransportProto,
                                              ServersUiController.processedServerId)
                } else {
                    PageController.goToPage(PageEnum.PageSetupWizardProtocols)
                }
            }
        }

        ButtonType {
            id: setupLaterButton

            Layout.fillWidth: true

            variant: ButtonType.Secondary

            visible: {
                if (PageController.isTriggeredByConnectButton()) {
                    PageController.setTriggeredByConnectButton(false)
                    return false
                }

                return true
            }

            text: qsTr("Skip setup")

            onClicked: {
                PageController.goToPage(PageEnum.PageSetupWizardInstalling)
                InstallController.addEmptyServer()
            }
        }
    }
}
