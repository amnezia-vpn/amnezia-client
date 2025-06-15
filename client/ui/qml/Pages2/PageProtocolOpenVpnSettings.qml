import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ContainerEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20

        onFocusChanged: {
            if (this.activeFocus) {
                listView.positionViewAtBeginning()
            }
        }
    }

    ListViewType {
        id: listView

        anchors.top: backButton.bottom
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.left: parent.left

        enabled: ServersModel.isProcessedServerHasWriteAccess()

        header: ColumnLayout {
            width: listView.width

            BaseHeaderType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("OpenVPN settings")
            }
        }

        model: ListModel {
            ListElement { type: "subnetHeader" }
            ListElement { type: "networkProtocolText" }
            ListElement { type: "protoSelector" }
            ListElement { type: "portTextField" }
            ListElement { type: "encryptionSection" }
            ListElement { type: "checkboxSection" }
            ListElement { type: "clientCommands" }
            ListElement { type: "serverCommands" }
        }

        delegate: DelegateChooser {

            role: "type"

            DelegateChoice {
                // property alias vpnAddressSubnetTextField: vpnAddressSubnetTextField
                roleValue: "subnetHeader"
                ColumnLayout {
                    width: listView.width

                    TextFieldWithHeaderType {
                        id: vpnAddressSubnetTextField

                        Layout.fillWidth: true
                        Layout.topMargin: 32
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16

                        headerText: qsTr("VPN address subnet")
                        textField.text: OpenVpnConfigModel.subnetAddress

                        textField.onEditingFinished: {
                            if (textField.text !== OpenVpnConfigModel.subnetAddress) {
                                OpenVpnConfigModel.subnetAddress = textField.text
                            }
                        }
                    }
                }
            }

            DelegateChoice {
                roleValue: "networkProtocolText"
                ColumnLayout {
                    width: listView.width

                    ParagraphTextType {
                        Layout.fillWidth: true
                        Layout.topMargin: 32
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16

                        text: qsTr("Network protocol")
                    }
                }
            }

            DelegateChoice {
                roleValue: "protoSelector"
                ColumnLayout {
                    width: listView.width

                    TransportProtoSelector {
                        id: transportProtoSelector

                        Layout.fillWidth: true
                        Layout.topMargin: 16
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16
                        rootWidth: root.width

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
            }

            DelegateChoice {
                roleValue: "portTextField"
                ColumnLayout {
                    width: listView.width

                    TextFieldWithHeaderType {
                        id: portTextField

                        Layout.fillWidth: true
                        Layout.topMargin: 40
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16

                        enabled: OpenVpnConfigModel.isPortEditable

                        headerText: qsTr("Port")
                        textField.text: OpenVpnConfigModel.port
                        textField.maximumLength: 5
                        textField.validator: IntValidator { bottom: 1; top: 65535 }

                        textField.onEditingFinished: {
                            if (textField.text !== OpenVpnConfigModel.port) {
                                OpenVpnConfigModel.port = textField.text
                            }
                        }
                    }
                }
            }

            DelegateChoice {
                roleValue: "encryptionSection"
                ColumnLayout {
                    width: listView.width

                    SwitcherType {
                        id: autoNegotiateEncryprionSwitcher

                        Layout.fillWidth: true
                        Layout.topMargin: 24
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16

                        text: qsTr("Auto-negotiate encryption")
                        checked: OpenVpnConfigModel.autoNegotiateEncryption

                        onCheckedChanged: {
                            if (checked !== OpenVpnConfigModel.autoNegotiateEncryprion) {
                                OpenVpnConfigModel.autoNegotiateEncryprion = checked
                            }
                        }
                    }

                    DropDownType {
                        id: hashDropDown

                        Layout.fillWidth: true
                        Layout.topMargin: 20
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16

                        enabled: !autoNegotiateEncryprionSwitcher.checked

                        descriptionText: qsTr("Hash")
                        headerText: qsTr("Hash")

                        drawerParent: root

                        listView: ListViewWithRadioButtonType {
                            id: hashListView

                            rootWidth: root.width

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

                            clickedFunction: function() {
                                hashDropDown.text = selectedText
                                OpenVpnConfigModel.hash = hashDropDown.text
                                hashDropDown.closeTriggered()
                            }

                            Component.onCompleted: {
                                hashDropDown.text = OpenVpnConfigModel.hash

                                for (var i = 0; i < hashListView.model.count; i++) {
                                    if (hashListView.model.get(i).name === hashDropDown.text) {
                                        currentIndex = i
                                    }
                                }
                            }
                        }
                    }

                    DropDownType {
                        id: cipherDropDown

                        Layout.fillWidth: true
                        Layout.topMargin: 16
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16

                        enabled: !autoNegotiateEncryprionSwitcher.checked

                        descriptionText: qsTr("Cipher")
                        headerText: qsTr("Cipher")

                        drawerParent: root

                        listView: ListViewWithRadioButtonType {
                            id: cipherListView

                            rootWidth: root.width

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

                            clickedFunction: function() {
                                cipherDropDown.text = selectedText
                                OpenVpnConfigModel.cipher = cipherDropDown.text
                                cipherDropDown.closeTriggered()
                            }

                            Component.onCompleted: {
                                cipherDropDown.text = OpenVpnConfigModel.cipher

                                for (var i = 0; i < cipherListView.model.count; i++) {
                                    if (cipherListView.model.get(i).name === cipherDropDown.text) {
                                        currentIndex = i
                                    }
                                }
                            }
                        }
                    }
                }
            }

            DelegateChoice {
                roleValue: "checkboxSection"
                ColumnLayout {
                    width: listView.width

                    Rectangle {
                        id: contentRect

                        Layout.fillWidth: true
                        Layout.topMargin: 32
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16
                        Layout.preferredHeight: checkboxLayout.implicitHeight
                        color: AmneziaStyle.color.onyxBlack
                        radius: 16

                        ColumnLayout {
                            id: checkboxLayout

                            anchors.fill: parent

                            CheckBoxType {
                                id: tlsAuthCheckBox

                                Layout.fillWidth: true

                                text: qsTr("TLS auth")
                                checked: OpenVpnConfigModel.tlsAuth

                                onCheckedChanged: {
                                    if (checked !== OpenVpnConfigModel.tlsAuth) {
                                        console.log("tlsAuth changed to: " + checked)
                                        OpenVpnConfigModel.tlsAuth = checked
                                    }
                                }
                            }

                            DividerType {}

                            CheckBoxType {
                                id: blockDnsCheckBox

                                Layout.fillWidth: true

                                text: qsTr("Block DNS requests outside of VPN")
                                checked: OpenVpnConfigModel.blockDns

                                onCheckedChanged: {
                                    if (checked !== OpenVpnConfigModel.blockDns) {
                                        OpenVpnConfigModel.blockDns = checked
                                    }
                                }
                            }
                        }
                    }
                }
            }

            DelegateChoice {
                roleValue: "clientCommands"
                ColumnLayout {

                    width: listView.width

                    SwitcherType {
                        id: additionalClientCommandsSwitcher

                        Layout.fillWidth: true
                        Layout.topMargin: 32
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16

                        checked: OpenVpnConfigModel.additionalClientCommands !== ""

                        text: qsTr("Additional client configuration commands")

                        onCheckedChanged: {
                            if (!checked) {
                                OpenVpnConfigModel.additionalClientCommands = ""
                            }
                            // listView.positionViewAtIndex(index, ListView.Beginning)
                        }
                    }

                    TextAreaType {
                        id: additionalClientCommandsTextArea

                        Layout.fillWidth: true
                        Layout.topMargin: 16
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16

                        visible: additionalClientCommandsSwitcher.checked

                        textAreaText: OpenVpnConfigModel.additionalClientCommands
                        placeholderText: qsTr("Commands:")

                        textArea.onEditingFinished: {
                            if (OpenVpnConfigModel.additionalClientCommands !== textAreaText) {
                                OpenVpnConfigModel.additionalClientCommands = textAreaText
                            }
                        }
                    }
                }
            }

            DelegateChoice {
                roleValue: "serverCommands"
                ColumnLayout {
                    width: listView.width

                    SwitcherType {
                        id: additionalServerCommandsSwitcher

                        Layout.fillWidth: true
                        Layout.topMargin: 16
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16

                        checked: OpenVpnConfigModel.additionalServerCommands !== ""

                        text: qsTr("Additional server configuration commands")

                        onCheckedChanged: {
                            if (!checked) {
                                OpenVpnConfigModel.additionalServerCommands = ""
                            }
                            // listView.positionViewAtIndex(index, ListView.Beginning)
                        }
                    }

                    TextAreaType {
                        id: additionalServerCommandsTextArea

                        Layout.fillWidth: true
                        Layout.topMargin: 16
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16

                        visible: additionalServerCommandsSwitcher.checked

                        textAreaText: OpenVpnConfigModel.additionalServerCommands
                        placeholderText: qsTr("Commands:")

                        textArea.onEditingFinished: {
                            if (OpenVpnConfigModel.additionalServerCommands !== textAreaText) {
                                OpenVpnConfigModel.additionalServerCommands = textAreaText
                            }
                        }
                    }
                }
            }
        }
        
        footer: ColumnLayout {
            width: listView.width

            BasicButtonType {
                id: saveRestartButton

                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.bottomMargin: 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                enabled: vpnAddressSubnetTextField.errorText === "" &&
                         portTextField.errorText === ""

                text: qsTr("Save")

                onClicked: function() {
                    forceActiveFocus()

                    var headerText = qsTr("Save settings?")
                    var descriptionText = qsTr("All users with whom you shared a connection with will no longer be able to connect to it.")
                    var yesButtonText = qsTr("Continue")
                    var noButtonText = qsTr("Cancel")

                    var yesButtonFunction = function() {
                        if (ConnectionController.isConnected && ServersModel.getDefaultServerData("defaultContainer") === ContainersModel.getProcessedContainerIndex()) {
                            PageController.showNotificationMessage(qsTr("Unable change settings while there is an active connection"))
                            return
                        }

                        PageController.goToPage(PageEnum.PageSetupWizardInstalling);
                        InstallController.updateContainer(OpenVpnConfigModel.getConfig())
                    }
                    var noButtonFunction = function() {
                        if (!GC.isMobile()) {
                            saveButton.forceActiveFocus()
                        }
                    }
                    showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                }

                Keys.onEnterPressed: saveButton.clicked()
                Keys.onReturnPressed: saveButton.clicked()
            }
        }
    }
}
