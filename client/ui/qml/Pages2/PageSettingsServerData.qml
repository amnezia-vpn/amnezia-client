import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import Style 1.0

import "../Controls2"
import "../Controls2/TextTypes"
import "../Components"
import "../Config"

PageType {
    id: root

    signal lastItemTabClickedSignal()

    property bool isServerWithWriteAccess: ServersUiController.isProcessedServerHasWriteAccess()
    property string selectedContainerValue: "all"

    Connections {
        target: InstallController

        function onScanServerFinished(isInstalledContainerFound) {
            var message = ""
            if (isInstalledContainerFound) {
                message = qsTr("All installed containers have been added to the application")
            } else {
                message = qsTr("No new installed containers found")
            }

            PageController.showErrorMessage(message)
        }

        function onRebootServerFinished(finishedMessage) {
            PageController.showNotificationMessage(finishedMessage)
        }
    }

    Connections {
        target: SettingsController
        function onChangeSettingsFinished(finishedMessage) {
            PageController.showNotificationMessage(finishedMessage)
        }
    }

    Connections {
        target: ServersUiController

        function onProcessedServerIdChanged() {
            root.isServerWithWriteAccess = ServersUiController.isProcessedServerHasWriteAccess()
        }
    }

    ListViewType {
        id: listView

        anchors.fill: parent

        model: serverActions

        delegate: ColumnLayout {
            width: listView.width

            LabelWithButtonType {
                Layout.fillWidth: true

                visible: isVisible

                text: title
                descriptionText: description
                textColor: tColor

                clickedFunction: function() {
                    clickedHandler()
                }
            }

            DividerType {
                visible: isVisible
            }
        }
    }

    property list<QtObject> serverActions: [
        check,
        backupSection,
        reboot,
        remove,
        clear,
        reset,
    ]

    QtObject {
        id: check

        property bool isVisible: root.isServerWithWriteAccess
        readonly property bool isBackupSection: false
        readonly property string title: qsTr("Check the server for previously installed Amnezia services")
        readonly property string description: qsTr("Add them to the application if they were not displayed")
        readonly property var tColor: AmneziaStyle.color.paleGray
        readonly property var clickedHandler: function() {
            PageController.showBusyIndicator(true)
            InstallController.scanServerForInstalledContainers(ServersUiController.processedServerId)
            PageController.showBusyIndicator(false)
        }
    }

    QtObject {
        id: backupSection

        property bool isVisible: root.isServerWithWriteAccess
        readonly property bool isBackupSection: false
        readonly property string title: qsTr("Backup")
        readonly property string description: qsTr("Local copy of VPN protocols, services, all server settings and users")
        readonly property var tColor: AmneziaStyle.color.paleGray
        readonly property var clickedHandler: function() {
            // Navigate to server backup page using PageController
            PageController.goToPage(PageEnum.PageSettingsServerBackup)
        }
    }

    QtObject {
        id: reboot

        property bool isVisible: root.isServerWithWriteAccess
        readonly property bool isBackupSection: false
        readonly property string title: qsTr("Reboot server")
        readonly property string description: ""
        readonly property var tColor: AmneziaStyle.color.vibrantRed
        readonly property var clickedHandler: function() {
            var headerText = qsTr("Do you want to reboot the server?")
            var descriptionText = qsTr("The reboot process may take approximately 30 seconds. Are you sure you wish to proceed?")
            var yesButtonText = qsTr("Continue")
            var noButtonText = qsTr("Cancel")

            var yesButtonFunction = function() {
                if (ServersUiController.isDefaultServerCurrentlyProcessed() && ConnectionController.isConnected) {
                    PageController.showNotificationMessage(qsTr("Cannot reboot server during active connection"))
                } else {
                    PageController.showBusyIndicator(true)
                    InstallController.rebootServer(ServersUiController.processedServerId)
                    PageController.showBusyIndicator(false)
                }
            }
            var noButtonFunction = function() {

            }

            showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
        }
    }

    QtObject {
        id: remove

        property bool isVisible: true
        readonly property bool isBackupSection: false
        readonly property string title: qsTr("Remove server from application")
        readonly property string description: ""
        readonly property var tColor: AmneziaStyle.color.vibrantRed
        readonly property var clickedHandler: function() {
            var headerText = qsTr("Do you want to remove the server from application?")
            var descriptionText = qsTr("All installed AmneziaVPN services will still remain on the server.")
            var yesButtonText = qsTr("Continue")
            var noButtonText = qsTr("Cancel")

            var yesButtonFunction = function() {
                if (ServersUiController.isDefaultServerCurrentlyProcessed() && ConnectionController.isConnected) {
                    PageController.showNotificationMessage(qsTr("Cannot remove server during active connection"))
                } else {
                    PageController.showBusyIndicator(true)
                    InstallController.removeServer(ServersUiController.processedServerId)
                    PageController.showBusyIndicator(false)
                }
            }
            var noButtonFunction = function() {

            }

            showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
        }
    }

    QtObject {
        id: clear

        property bool isVisible: root.isServerWithWriteAccess
        readonly property bool isBackupSection: false
        readonly property string title: qsTr("Clear server from Amnezia software")
        readonly property string description: ""
        readonly property var tColor: AmneziaStyle.color.vibrantRed
        readonly property var clickedHandler: function() {
            var headerText = qsTr("Do you want to clear server from Amnezia software?")
            var descriptionText = qsTr("All users whom you shared a connection with will no longer be able to connect to it.")
            var yesButtonText = qsTr("Continue")
            var noButtonText = qsTr("Cancel")

            var yesButtonFunction = function() {
                if (ServersUiController.isDefaultServerCurrentlyProcessed() && ConnectionController.isConnected) {
                    PageController.showNotificationMessage(qsTr("Cannot clear server from Amnezia software during active connection"))
                } else {
                    PageController.goToPage(PageEnum.PageDeinstalling)
                    InstallController.removeAllContainers(ServersUiController.processedServerId)
                }
            }
            var noButtonFunction = function() {

            }

            showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
        }
    }

    QtObject {
        id: reset

        property bool isVisible: ServersUiController.isServerFromApi(ServersUiController.processedServerId)
        readonly property bool isBackupSection: false
        readonly property string title: qsTr("Reset API config")
        readonly property string description: ""
        readonly property var tColor: AmneziaStyle.color.vibrantRed
        readonly property var clickedHandler: function() {
            var headerText = qsTr("Do you want to reset API config?")
            var descriptionText = ""
            var yesButtonText = qsTr("Continue")
            var noButtonText = qsTr("Cancel")

            var yesButtonFunction = function() {
                if (ServersUiController.isDefaultServerCurrentlyProcessed() && ConnectionController.isConnected) {
                    PageController.showNotificationMessage(qsTr("Cannot reset API config during active connection"))
                } else {
                    PageController.showBusyIndicator(true)
                    SubscriptionUiController.removeApiConfig(ServersUiController.processedServerId)
                    PageController.showBusyIndicator(false)
                }
            }
            var noButtonFunction = function() {

            }

            showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
        }
    }


    // ============ Backup Functions ============

    function getServerCredentials() {
        var index = ServersModel.processedIndex
        return ServersModel.getServerCredentials(index)
    }

    function getSelectedContainerValue() {
        return root.selectedContainerValue
    }

    property bool downloadAfterCreate: false

    function createBackup(shouldDownload) {
        downloadAfterCreate = shouldDownload || false

        var headerText = shouldDownload ?
            qsTr("Create backup and download to device?") :
            qsTr("Create server configuration backup?")
        var descriptionText = shouldDownload ?
            qsTr("Backup will be created on server and automatically downloaded to your device") :
            qsTr("This will create a backup of your server containers configuration on the server")
        var yesButtonText = qsTr("Create")
        var noButtonText = qsTr("Cancel")

        var yesButtonFunction = function() {
            PageController.showBusyIndicator(true)
            var credentials = getServerCredentials()
            var containerValue = getSelectedContainerValue()
            if (containerValue === "all") {
                ServersBackupController.createBackup(credentials)
            } else {
                ServersBackupController.createBackupByName(credentials, containerValue)
            }
        }
        var noButtonFunction = function() {}

        showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
    }

    property string selectedBackupForRestore: ""

    function restoreBackup() {
        var filter = GC.isMobile() ? "*.gz" : "Backup files (*.tar.gz *.backup)"
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

        var headerText = qsTr("Restore server configuration?")
        var descriptionText = qsTr("Selected backup will be uploaded to server and restored. Current configuration will be overwritten. All containers will be restarted.")
        var yesButtonText = qsTr("Restore")
        var noButtonText = qsTr("Cancel")

        var yesButtonFunction = function() {
            PageController.showBusyIndicator(true)
            var credentials = getServerCredentials()
            ServersBackupController.uploadBackup(credentials, selectedBackupForRestore)
        }
        var noButtonFunction = function() {
            selectedBackupForRestore = ""
        }

        showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
    }

    function manageBackups() {
        PageController.showNotificationMessage(qsTr("Backup management page will be available in the next update"))
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
            if (lastCreatedBackupFilename && lastCreatedBackupFilename.length > 0) {
                var credentials = getServerCredentials()
                ServersBackupController.deleteBackup(credentials, lastCreatedBackupFilename)
            }
            PageController.showNotificationMessage(qsTr("Backup downloaded successfully!\n\nSaved to:\n%1").arg(localPath))
        }

        function onBackupUploaded(serverPath) {
            PageController.showNotificationMessage(qsTr("Backup uploaded. Restoring configuration..."))
            var backupFilename = serverPath.split('/').pop()
            lastUploadedBackupFilename = backupFilename
            var credentials = getServerCredentials()
            ServersBackupController.restoreBackup(credentials, backupFilename)
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
