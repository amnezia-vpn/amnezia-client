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

    property string backupFilePath: ""
    property string backupFileName: ""
    property string serverName: ""
    property string serverIp: ""
    property bool isFromSetupWizard: false

    // Credentials for setup wizard (when server is not yet added to ServersModel)
    property string wizardHostname: ""
    property string wizardUsername: ""
    property string wizardSecretData: ""

    property bool restoreReplaceMode: false

    FlickableType {
        id: flickable

        anchors.top: parent.top
        anchors.bottom: parent.bottom

        contentHeight: content.implicitHeight + 16

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 0

            NavigationBarType {
                Layout.fillWidth: true

                title: qsTr("Restore from backup")
                description: (serverIp && serverIp.length > 0) ? qsTr("%1 on %2").arg(backupFileName).arg(serverIp) : backupFileName
            }

            ListItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Add data from backup")
                subtitle: qsTr("If the same protocols are already installed on the server, they will be updated. Created users and access will be saved")
                maximumLineCount: 3
                trailingIconSource: "qrc:/images/controls/chevron-right.svg"

                onClicked: startRestore(false)
            }

            ListItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Replace")
                subtitle: qsTr("All installed protocols, users and their access will not be saved")
                destructive: true
                trailingIconSource: "qrc:/images/controls/chevron-right.svg"
                showSeparator: false

                onClicked: startRestore(true)
            }
        }
    }

    function startRestore(replaceMode) {
        restoreReplaceMode = replaceMode
        PageController.showBusyIndicator(true)

        BackupController.startRestore(
            isFromSetupWizard,
            backupFilePath,
            replaceMode,
            wizardHostname || "",
            wizardUsername || "",
            wizardSecretData || ""
        )
    }

    Connections {
        target: BackupController

        function onBackupRestored() {
            if (isFromSetupWizard) {
                BackupController.setDefaultServerAfterRestore()
            } else {
                PageController.showBusyIndicator(false)
                navigateToRestoredPage()
            }
        }

        function onDefaultServerAndContainerSet() {
            PageController.showBusyIndicator(false)
            navigateToRestoredPage()
        }

        function onErrorOccurred(errorMessage) {
            PageController.showBusyIndicator(false)
            PageController.showErrorMessage(qsTr("Backup restore error: %1").arg(errorMessage))
        }
    }

    function navigateToRestoredPage() {
        var actualServerName = serverName
        if (root.isFromSetupWizard && ServersUiController.getServersCount() > 0) {
            var lastServerId = ServersUiController.getServerId(ServersUiController.getServersCount() - 1)
            actualServerName = ServersUiController.serverName(lastServerId) || qsTr("Server")
        } else if (!serverName || serverName.length === 0) {
            actualServerName = ServersUiController.serverName(ServersUiController.processedServerId) || qsTr("Server")
        }

        var parentItem = root.parent

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
}
