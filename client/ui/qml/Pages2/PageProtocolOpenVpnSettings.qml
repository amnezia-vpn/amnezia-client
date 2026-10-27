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

    readonly property bool isSaveButtonEnabled: vpnAddressSubnetTextField.errorText === "" &&
                                                portTextField.errorText === ""
    property bool hasAdditionalClientCommands: false
    property bool hasAdditionalServerCommands: false

    Component.onCompleted: {
        root.hasAdditionalClientCommands = OpenVpnConfigModel.additionalClientCommands !== ""
        root.hasAdditionalServerCommands = OpenVpnConfigModel.additionalServerCommands !== ""
    }

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

            spacing: 0

            NavigationBarType {
                id: header

                Layout.fillWidth: true

                title: qsTr("OpenVPN Settings")
            }

            ColumnLayout {
                Layout.fillWidth: true

                spacing: 24

                TextFieldType {
                    id: vpnAddressSubnetTextField

                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16

                    enabled: flickable.enabled

                    headerText: qsTr("VPN address subnet")
                    textField.text: OpenVpnConfigModel.subnetAddress

                    textField.onTextEdited: OpenVpnConfigModel.subnetAddress = textField.text

                    checkEmptyText: true
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16

                    spacing: 12

                    BodyTextType {
                        Layout.fillWidth: true

                        text: qsTr("Network protocol")
                        color: AmneziaStyle.color.textTertiary
                    }

                    TransportProtoSelector {
                        id: transportProtoSelector

                        Layout.fillWidth: true

                        enabled: OpenVpnConfigModel.isTransportProtoEditable

                        currentIndex: {
                            return OpenVpnConfigModel.transportProto === "tcp" ? 1 : 0
                        }

                        onCurrentIndexChanged: {
                            if (OpenVpnConfigModel.transportProto === "tcp" && currentIndex === 0) {
                                OpenVpnConfigModel.transportProto = "udp"
                            } else if (OpenVpnConfigModel.transportProto === "udp" && currentIndex === 1) {
                                OpenVpnConfigModel.transportProto = "tcp"
                            }
                        }
                    }
                }

                TextFieldType {
                    id: portTextField

                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16

                    enabled: flickable.enabled

                    headerText: qsTr("Port")
                    textField.text: OpenVpnConfigModel.port
                    textField.maximumLength: 5
                    textField.validator: IntValidator { bottom: 1; top: 65535 }

                    textField.onTextEdited: OpenVpnConfigModel.port = textField.text

                    checkEmptyText: true
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16

                    spacing: 12

                    SwitchType {
                        id: autoNegotiateEncryprionSwitcher

                        Layout.fillWidth: true

                        text: qsTr("Auto-negotiate encryption")
                        checked: OpenVpnConfigModel.autoNegotiateEncryprion

                        onToggled: function() {
                            OpenVpnConfigModel.autoNegotiateEncryprion = checked
                        }
                    }

                    DropDownType {
                        id: hashDropDown

                        Layout.fillWidth: true

                        enabled: !autoNegotiateEncryprionSwitcher.checked

                        descriptionText: qsTr("Hash")
                        headerText: qsTr("Hash")

                        listView: RadioListType {
                            id: hashListView

                            model: ListModel {
                                ListElement { name : qsTr("SHA512") }
                                ListElement { name : qsTr("SHA384") }
                                ListElement { name : qsTr("SHA256") }
                                ListElement { name : qsTr("SHA3-512") }
                                ListElement { name : qsTr("SHA3-384") }
                                ListElement { name : qsTr("SHA3-256") }
                                ListElement { name : qsTr("whirlpool") }
                                ListElement { name : qsTr("BLAKE2b512") }
                                ListElement { name : qsTr("BLAKE2s256") }
                                ListElement { name : qsTr("SHA1") }
                            }

                            function updateSelectedIndex() {
                                hashDropDown.text = OpenVpnConfigModel.hash
                                for (var i = 0; i < hashListView.model.count; i++) {
                                    if (hashListView.model.get(i).name === OpenVpnConfigModel.hash) {
                                        selectedIndex = i
                                        break
                                    }
                                }
                            }

                            onActivated: {
                                hashDropDown.text = selectedText
                                OpenVpnConfigModel.hash = hashDropDown.text
                                hashDropDown.close()
                            }

                            Component.onCompleted: {
                                updateSelectedIndex()
                            }
                        }

                        Connections {
                            target: OpenVpnConfigModel
                            function onChanged() {
                                hashListView.updateSelectedIndex()
                            }
                        }
                    }

                    DropDownType {
                        id: cipherDropDown

                        Layout.fillWidth: true

                        enabled: !autoNegotiateEncryprionSwitcher.checked

                        descriptionText: qsTr("Cipher")
                        headerText: qsTr("Cipher")

                        listView: RadioListType {
                            id: cipherListView

                            model: ListModel {
                                ListElement { name : qsTr("AES-256-GCM") }
                                ListElement { name : qsTr("AES-192-GCM") }
                                ListElement { name : qsTr("AES-128-GCM") }
                                ListElement { name : qsTr("AES-256-CBC") }
                                ListElement { name : qsTr("AES-192-CBC") }
                                ListElement { name : qsTr("AES-128-CBC") }
                                ListElement { name : qsTr("ChaCha20-Poly1305") }
                                ListElement { name : qsTr("ARIA-256-CBC") }
                                ListElement { name : qsTr("CAMELLIA-256-CBC") }
                                ListElement { name : qsTr("none") }
                            }

                            function updateSelectedIndex() {
                                cipherDropDown.text = OpenVpnConfigModel.cipher
                                for (var i = 0; i < cipherListView.model.count; i++) {
                                    if (cipherListView.model.get(i).name === OpenVpnConfigModel.cipher) {
                                        selectedIndex = i
                                        break
                                    }
                                }
                            }

                            onActivated: {
                                cipherDropDown.text = selectedText
                                OpenVpnConfigModel.cipher = cipherDropDown.text
                                cipherDropDown.close()
                            }

                            Component.onCompleted: {
                                updateSelectedIndex()
                            }
                        }

                        Connections {
                            target: OpenVpnConfigModel
                            function onChanged() {
                                cipherListView.updateSelectedIndex()
                            }
                        }
                    }
                }

                Rectangle {
                    id: contentRect

                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.preferredHeight: checkboxLayout.implicitHeight + 16

                    color: AmneziaStyle.color.bgSurface
                    radius: 16

                    ColumnLayout {
                        id: checkboxLayout

                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter

                        spacing: 0

                        CheckBoxType {
                            id: tlsAuthCheckBox

                            Layout.fillWidth: true
                            Layout.leftMargin: 16
                            Layout.rightMargin: 12

                            topPadding: 16
                            bottomPadding: 16
                            spacing: 20

                            text: qsTr("TLS auth")
                            checked: OpenVpnConfigModel.tlsAuth

                            onCheckedChanged: {
                                OpenVpnConfigModel.tlsAuth = checked
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 1

                            color: AmneziaStyle.color.borderFaint
                        }

                        CheckBoxType {
                            id: blockDnsCheckBox

                            Layout.fillWidth: true
                            Layout.leftMargin: 16
                            Layout.rightMargin: 12

                            topPadding: 16
                            bottomPadding: 16
                            spacing: 20

                            text: qsTr("Block DNS requests outside of VPN")
                            checked: OpenVpnConfigModel.blockDns

                            onCheckedChanged: {
                                OpenVpnConfigModel.blockDns = checked
                            }
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16

                    spacing: 12

                    SwitchType {
                        id: additionalClientCommandsSwitcher

                        Layout.fillWidth: true

                        checked: root.hasAdditionalClientCommands

                        text: qsTr("Additional client configuration commands")

                        onToggled: function() {
                            root.hasAdditionalClientCommands = checked
                            if (!checked) {
                                OpenVpnConfigModel.additionalClientCommands = ""
                            }
                        }
                    }

                    TextAreaType {
                        id: additionalClientCommandsTextArea

                        Layout.fillWidth: true

                        visible: additionalClientCommandsSwitcher.checked

                        textAreaText: OpenVpnConfigModel.additionalClientCommands
                        placeholderText: qsTr("Commands:")

                        textArea.onTextChanged: OpenVpnConfigModel.additionalClientCommands = textAreaText
                    }

                    SwitchType {
                        id: additionalServerCommandsSwitcher

                        Layout.fillWidth: true

                        checked: root.hasAdditionalServerCommands

                        text: qsTr("Additional server configuration commands")

                        onToggled: function() {
                            root.hasAdditionalServerCommands = checked
                            if (!checked) {
                                OpenVpnConfigModel.additionalServerCommands = ""
                            }
                        }
                    }

                    TextAreaType {
                        id: additionalServerCommandsTextArea

                        Layout.fillWidth: true

                        visible: additionalServerCommandsSwitcher.checked

                        textAreaText: OpenVpnConfigModel.additionalServerCommands
                        placeholderText: qsTr("Commands:")

                        textArea.onTextChanged: OpenVpnConfigModel.additionalServerCommands = textAreaText
                    }
                }
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
                InstallController.updateServerConfig(ServersUiController.processedServerId, ServersUiController.processedContainerIndex, ProtocolEnum.OpenVpn)
            }
            var noButtonFunction = function() {}
            Dialogs.askQuestion(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
        }
    }
}
