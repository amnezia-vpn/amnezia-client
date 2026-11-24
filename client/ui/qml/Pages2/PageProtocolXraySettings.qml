import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import ProtocolEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    focusOnActivation: false

    function save() {
        const errors = XraySettingsModel.validationErrors()
        console.log("[XrayUI][Settings] save clicked, errors", errors.length, "preset", XraySettingsModel.presetId)
        if (errors.length > 0) {
            PageController.showErrorMessage(errors.join("\n"))
            return
        }
        saveDialog.open()
    }

    function applySave() {
        if (ConnectionController.isConnected && ServersUiController.serverDefaultContainer(ServersUiController.defaultServerId) === ServersUiController.processedContainerIndex) {
            console.log("[XrayUI][Settings] save blocked: active connection")
            PageController.showNotificationMessage(qsTr("Unable change settings while there is an active connection"))
            return
        }

        const touchesServer = XraySettingsModel.touchesServer()
        console.log("[XrayUI][Settings] save confirmed, touchesServer", touchesServer, "port", XraySettingsModel.port)
        if (touchesServer) {
            PageController.goToPage(PageEnum.PageSetupWizardInstalling)
        }
        InstallController.updateServerConfig(ServersUiController.processedServerId, ServersUiController.processedContainerIndex, ProtocolEnum.Xray, touchesServer)
    }

    Component.onCompleted: console.log("[XrayUI][Settings] opened, preset", XraySettingsModel.presetId, "edited", XraySettingsModel.presetEdited, "port", XraySettingsModel.port, "unsaved", XraySettingsModel.hasUnsavedChanges)
    Component.onDestruction: console.log("[XrayUI][Settings] closed, unsaved", XraySettingsModel.hasUnsavedChanges)

    Connections {
        target: XraySettingsModel

        function onChanged() {
            console.log("[XrayUI][Settings] model changed, preset", XraySettingsModel.presetId, "edited", XraySettingsModel.presetEdited, "port", XraySettingsModel.port, "unsaved", XraySettingsModel.hasUnsavedChanges)
        }
    }

    Connections {
        target: InstallController

        function onXrayKeyMigrationNeedsConfirm(message) {
            console.log("[XrayUI][Settings] key migration confirmation requested")
            PageController.showBusyIndicator(false)
            Dialogs.askQuestion(
                qsTr("Recreate XRay with new keys?"),
                message,
                qsTr("Continue"),
                qsTr("Cancel"),
                function() {
                    console.log("[XrayUI][Settings] key migration accepted")
                    InstallController.updateServerConfig(ServersUiController.processedServerId,
                                                         ServersUiController.processedContainerIndex,
                                                         ProtocolEnum.Xray, true, true)
                },
                function() {
                    console.log("[XrayUI][Settings] key migration declined")
                })
        }
    }

    AlertDialogType {
        id: saveDialog

        headerText: qsTr("Save settings?")
        descriptionText: qsTr("The server configuration will be updated. Everyone you shared this connection with will lose access. Share it again after saving")
        confirmButtonText: qsTr("Save")

        onOpened: console.log("[XrayUI][Settings] save dialog opened")
        onConfirmed: root.applySave()
    }

    AlertDialogType {
        id: resetDialog

        headerText: qsTr("Reset settings?")
        descriptionText: qsTr("All XRay settings will be restored to defaults")
        confirmButtonText: qsTr("Reset")

        onOpened: console.log("[XrayUI][Settings] reset dialog opened")
        onConfirmed: {
            console.log("[XrayUI][Settings] reset confirmed, preset", XraySettingsModel.presetId)
            XraySettingsModel.resetToPreset()
        }
    }

    FlickableType {
        id: flickable

        anchors.top: parent.top
        anchors.bottom: buttons.top
        anchors.bottomMargin: 24

        contentHeight: content.implicitHeight

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 0

            NavigationBarType {
                Layout.fillWidth: true

                title: qsTr("XRay VLESS settings")
            }

            LinkButtonType {
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                size: LinkButtonType.Medium
                horizontalPadding: 14
                verticalPadding: 8

                text: qsTr("More about settings")

                onClicked: {
                    console.log("[XrayUI][Settings] more about settings clicked")
                    Qt.openUrlExternally("https://docs.amnezia.org")
                }
            }

            ListItemType {
                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Preset")
                subtitle: XraySettingsModel.presetEdited ? XraySettingsModel.presetName + " · " + qsTr("edited")
                                                         : XraySettingsModel.presetName
                trailingIconSource: "qrc:/images/controls/chevron-right.svg"

                onClicked: {
                    console.log("[XrayUI][Settings] preset row clicked")
                    PageController.goToPage(PageEnum.PageProtocolXrayPresets)
                }
            }

            ListItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Edit config")
                subtitle: "server.json · " + qsTr("port %1").arg(XraySettingsModel.port)
                trailingIconSource: "qrc:/images/controls/chevron-right.svg"

                onClicked: {
                    console.log("[XrayUI][Settings] edit config clicked")
                    XraySettingsModel.openEditor()
                    PageController.goToPage(PageEnum.PageProtocolXrayConfigEditor)
                }
            }
        }
    }

    ColumnLayout {
        id: buttons

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        anchors.bottomMargin: 8 + PageController.safeAreaBottomMargin

        spacing: 12

        ButtonType {
            Layout.fillWidth: true

            enabled: XraySettingsModel.hasUnsavedChanges
            text: qsTr("Save")

            onClicked: {
                forceActiveFocus()
                root.save()
            }
        }

        ButtonType {
            Layout.fillWidth: true

            variant: ButtonType.Secondary
            text: qsTr("Reset settings")

            onClicked: {
                console.log("[XrayUI][Settings] reset clicked, canReset", XraySettingsModel.canReset)
                resetDialog.open()
            }
        }
    }
}
