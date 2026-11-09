import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

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

    readonly property bool isSaveButtonEnabled: portTextField.errorText === "" && vpnAddressSubnetTextField.errorText === ""

    FlickableType {
        id: flickable

        anchors.top: parent.top
        anchors.bottom: saveButton.top
        anchors.bottomMargin: 24

        enabled: ServersUiController.isProcessedServerHasWriteAccess()

        contentHeight: content.implicitHeight

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 12

            NavigationBarType {
                Layout.fillWidth: true
                Layout.bottomMargin: -content.spacing

                title: qsTr("WG settings")
            }

            TextFieldType {
                id: vpnAddressSubnetTextField

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("VPN address subnet")
                textField.text: WireGuardConfigModel.subnetAddress

                textField.onTextEdited: WireGuardConfigModel.subnetAddress = textField.text

                checkEmptyText: true
            }

            TextFieldType {
                id: portTextField

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("Port")
                textField.text: WireGuardConfigModel.port
                textField.maximumLength: 5
                textField.validator: IntValidator { bottom: 1; top: 65535 }

                textField.onTextEdited: WireGuardConfigModel.port = textField.text

                checkEmptyText: true
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

        enabled: flickable.enabled && root.isSaveButtonEnabled

        text: qsTr("Save")

        onClicked: function() {
            var headerText = qsTr("Save settings?")
            var descriptionText = qsTr("All users with whom you shared a connection with will no longer be able to connect to it.")
            var yesButtonText = qsTr("Continue")
            var noButtonText = qsTr("Cancel")

            var yesButtonFunction = function() {
                if (ConnectionController.isConnected && ServersUiController.serverDefaultContainer(ServersUiController.defaultServerId) === ServersUiController.processedContainerIndex) {
                    PageController.showNotificationMessage(qsTr("Unable change settings while there is an active connection"))
                    return
                }

                PageController.goToPage(PageEnum.PageSetupWizardInstalling);
                InstallController.updateServerConfig(ServersUiController.processedServerId, ServersUiController.processedContainerIndex, ProtocolEnum.WireGuard)
            }
            var noButtonFunction = function() {}
            Dialogs.askQuestion(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
        }
    }
}
