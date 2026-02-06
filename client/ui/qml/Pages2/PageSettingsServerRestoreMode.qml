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
    
    // Credentials for setup wizard (when server is not yet added to ServersModel)
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
                    // Show only filename and IP address, without server name
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
                        startRestore(false) // false = add mode
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
                        startRestore(true) // true = replace mode
                    }
                }
            }
        }
    }

    property bool restoreReplaceMode: false

    function startRestore(replaceMode) {
        restoreReplaceMode = replaceMode
        PageController.showBusyIndicator(true)
        
        // Call universal C++ method that will determine how to perform restore
        ServersBackupController.startRestore(
            isFromSetupWizard,
            backupFilePath,
            replaceMode,
            wizardHostname || "",
            wizardUsername || "",
            wizardSecretData || ""
        )
    }

    property string lastUploadedBackupFilename: ""

    Connections {
        target: ServersBackupController

        function onBackupRestored() {
            console.log("  onBackupRestored, isFromSetupWizard:", isFromSetupWizard)
            
            // For setup wizard, call C++ method to set default server and container
            if (isFromSetupWizard) {
                ServersBackupController.setDefaultServerAfterRestore(true)
            } else {
                // For regular mode, navigate directly
                PageController.showBusyIndicator(false)
                navigateToRestoredPage()
            }
        }
        
        function onDefaultServerAndContainerSet() {
            console.log("  onDefaultServerAndContainerSet - navigating to restored page")
            // C++ has set default server and container, navigate to result page
            PageController.showBusyIndicator(false)
            navigateToRestoredPage()
        }

        function onErrorOccurred(errorMessage, errorCode) {
            PageController.showBusyIndicator(false)
            PageController.showErrorMessage(qsTr("Backup restore error: %1").arg(errorMessage))
        }
    }
    
    
    function navigateToRestoredPage() {
        // Navigate to successful restore page
        // Get actual server name from model
        var actualServerName = serverName
        if (root.isFromSetupWizard && ServersModel.getServersCount() > 0) {
            var serverIdx = ServersModel.getServersCount() - 1
            var oldProcessedIndex = ServersModel.processedIndex
            ServersModel.processedIndex = serverIdx
            actualServerName = ServersModel.getProcessedServerData("name") || qsTr("Server")
            ServersModel.processedIndex = oldProcessedIndex
        } else if (!serverName || serverName.length === 0) {
            // If name not provided, get from processedIndex
            actualServerName = ServersModel.getProcessedServerData("name") || qsTr("Server")
        }
        
        var parentItem = root.parent
        
        // For setup wizard use regular StackView
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
            // For management menu, find tabBarStackView
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
