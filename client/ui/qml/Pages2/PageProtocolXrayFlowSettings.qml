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

    readonly property bool visionAllowed: (XrayConfigModel.transport === "" || XrayConfigModel.transport === "raw")
                                          && (XrayConfigModel.security === "tls" || XrayConfigModel.security === "reality")

    FlickableType {
        id: flickable

        anchors.top: parent.top
        anchors.bottom: saveButton.visible ? saveButton.top : parent.bottom
        anchors.bottomMargin: saveButton.visible ? 24 : 0

        enabled: ServersUiController.isProcessedServerHasWriteAccess()

        contentHeight: content.implicitHeight

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 0

            NavigationBarType {
                Layout.fillWidth: true

                title: qsTr("Flow")
            }

            RadioItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                text: qsTr("Empty")
                checked: XrayConfigModel.flow === "" || !root.visionAllowed
                onClicked: XrayConfigModel.flow = ""
            }

            RadioItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                text: "xtls-rprx-vision"
                enabled: root.visionAllowed
                checked: XrayConfigModel.flow === "xtls-rprx-vision" && root.visionAllowed
                onClicked: XrayConfigModel.flow = "xtls-rprx-vision"
            }

            RadioItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                text: "xtls-rprx-vision-udp443"
                subtitle: qsTr("For apps that require QUIC. Browsers may stop loading.")
                enabled: root.visionAllowed
                checked: XrayConfigModel.flow === "xtls-rprx-vision-udp443" && root.visionAllowed
                onClicked: XrayConfigModel.flow = "xtls-rprx-vision-udp443"
            }

            SmallTextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 12
                visible: !root.visionAllowed
                color: AmneziaStyle.color.textAccent
                text: qsTr("xtls-rprx-vision is available only with the RAW (TCP) transport and TLS or Reality security.")
            }

            Item {
                Layout.preferredHeight: saveButton.visible ? 0 : 16 + PageController.safeAreaBottomMargin
            }
        }
    }

    ButtonType {
        id: saveButton

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        anchors.bottomMargin: 8 + PageController.safeAreaBottomMargin

        visible: flickable.enabled
        enabled: XrayConfigModel.hasUnsavedChanges
        text: qsTr("Save")
        onClicked: {
            saveButton.forceActiveFocus()
            var errs = XrayConfigModel.validationErrors()
            if (errs.length > 0) {
                PageController.showErrorMessage(errs.join("\n"))
                return
            }
            var headerText = qsTr("Save settings?")
            var descriptionText = XrayConfigModel.saveDescription("")
            var yesButtonText = qsTr("Continue")
            var noButtonText = qsTr("Cancel")
            var yesButtonFunction = function () {
                if (ConnectionController.isConnected && ServersUiController.serverDefaultContainer(ServersUiController.defaultServerId) === ServersUiController.processedContainerIndex) {
                    PageController.showNotificationMessage(qsTr("Unable change settings while there is an active connection"))
                    return
                }
                var touchesServer = XrayConfigModel.pendingChangeTouchesServer("")
                if (touchesServer)
                    PageController.goToPage(PageEnum.PageSetupWizardInstalling)
                InstallController.updateServerConfig(ServersUiController.processedServerId, ServersUiController.processedContainerIndex, ProtocolEnum.Xray, touchesServer)
            }
            var noButtonFunction = function () {}
            Dialogs.askQuestion(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
        }
    }
}
