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

    ListViewType {
        id: listView

        anchors.top: parent.top
        anchors.bottom: saveButton.top
        anchors.bottomMargin: 16
        anchors.right: parent.right
        anchors.left: parent.left

        header: NavigationBarType {
            width: listView.width
            height: implicitHeight + 16

            title: qsTr("WG settings")
        }

        model: WireGuardConfigModel

        delegate: ColumnLayout {
            width: listView.width

            property alias mtuTextField: mtuTextField
            property bool isSaveButtonEnabled: mtuTextField.errorText === ""

            spacing: 0

            TextFieldType {
                id: mtuTextField

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("MTU")
                textField.text: clientMtu
                textField.validator: IntValidator { bottom: 576; top: 65535 }

                textField.onEditingFinished: {
                    if (textField.text !== clientMtu) {
                        clientMtu = textField.text
                    }
                }
                checkEmptyText: true
            }

            BodyTextType {
                Layout.fillWidth: true
                Layout.topMargin: 12
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Server settings")
                color: AmneziaStyle.color.textTertiary
            }

            TextFieldType {
                id: portTextField

                Layout.fillWidth: true
                Layout.topMargin: 8
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                enabled: false

                headerText: qsTr("Port")
                textField.text: port
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
        anchors.bottomMargin: 16 + PageController.safeAreaBottomMargin

        enabled: listView.currentItem !== null && listView.currentItem.isSaveButtonEnabled

        text: qsTr("Save")

        onClicked: {
            var headerText = qsTr("Save settings?")
            var descriptionText = qsTr("Only the settings for this device will be changed")
            var yesButtonText = qsTr("Continue")
            var noButtonText = qsTr("Cancel")

            var yesButtonFunction = function() {
                if (ConnectionController.isConnected && ServersUiController.serverDefaultContainer(ServersUiController.defaultServerId) === ServersUiController.processedContainerIndex) {
                    PageController.showNotificationMessage(qsTr("Unable change settings while there is an active connection"))
                    return
                }

                InstallController.updateClientConfig(ServersUiController.processedServerId, ServersUiController.processedContainerIndex, ProtocolEnum.WireGuard)
            }
            var noButtonFunction = function() {}
            showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
        }
    }
}
