import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ContainerEnum 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    defaultActiveFocusItem: listview.currentItem.vpnAddressSubnetTextField.textField

    ColumnLayout {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        anchors.topMargin: 20

        BackButtonType {
        }
    }

    FlickableType {
        id: fl
        anchors.top: backButton.bottom
        anchors.bottom: parent.bottom
        contentHeight: content.implicitHeight

        Column {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            enabled: ServersModel.isProcessedServerHasWriteAccess()

            ListView {
                id: listview

                width: parent.width
                height: listview.contentItem.height

                clip: true
                interactive: false

                model: OpenVpnConfigModel             

                delegate: Item {
                    implicitWidth: listview.width
                    implicitHeight: col.implicitHeight

                    property alias vpnAddressSubnetTextField: vpnAddressSubnetTextField

                    ColumnLayout {
                        id: col

                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right

                        anchors.leftMargin: 16
                        anchors.rightMargin: 16

                        spacing: 0

                        HeaderType {
                            Layout.fillWidth: true

                            headerText: qsTr("OpenVPN settings")
                        }

                        TextFieldWithHeaderType {
                            id: vpnAddressSubnetTextField

                            Layout.fillWidth: true
                            Layout.topMargin: 32

                            headerText: qsTr("VPN address subnet")
                            textFieldText: subnetAddress

                            textField.onEditingFinished: {
                                if (textFieldText !== subnetAddress) {
                                    subnetAddress = textFieldText
                                }
                            }

                            KeyNavigation.tab: portTextField.enabled ? portTextField.textField : saveRestartButton
                        }

                        ParagraphTextType {
                            Layout.fillWidth: true
                            Layout.topMargin: 32

                            text: qsTr("Network protocol")
                        }

                        TransportProtoSelector {
                            Layout.fillWidth: true
                            Layout.topMargin: 16
                            rootWidth: root.width

                            enabled: isTransportProtoEditable

                            currentIndex: {
                                return transportProto === "tcp" ? 1 : 0
                            }

                            onCurrentIndexChanged: {
                                if (transportProto === "tcp" && currentIndex === 0) {
                                    transportProto = "udp"
                                } else if (transportProto === "udp" && currentIndex === 1) {
                                    transportProto = "tcp"
                                }
                            }
                        }

                        TextFieldWithHeaderType {
                            id: portTextField


                            Layout.fillWidth: true
                            Layout.topMargin: 40

                            enabled: isPortEditable

                            headerText: qsTr("Port")
                            textFieldText: port
                            textField.maximumLength: 5
                            textField.validator: IntValidator { bottom: 1; top: 65535 }

                            textField.onEditingFinished: {
                                if (textFieldText !== port) {
                                    port = textFieldText
                                }
                            }

                            KeyNavigation.tab: saveRestartButton
                        }

                        SwitcherType {
                            id: autoNegotiateEncryprionSwitcher

                            Layout.fillWidth: true
                            Layout.topMargin: 24

                            text: qsTr("Auto-negotiate encryption")
                            checked: autoNegotiateEncryprion

                            onCheckedChanged: {
                                if (checked !== autoNegotiateEncryprion) {
                                    autoNegotiateEncryprion = checked
                                }
                            }
                        }

                        DropDownType {
                            id: hashDropDown
                            Layout.fillWidth: true
                            Layout.topMargin: 20

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
                                    hash = hashDropDown.text
                                    hashDropDown.close()
                                }

                                Component.onCompleted: {
                                    hashDropDown.text = hash

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
                                    cipher = cipherDropDown.text
                                    cipherDropDown.close()
                                }

                                Component.onCompleted: {
                                    cipherDropDown.text = cipher

                                    for (var i = 0; i < cipherListView.model.count; i++) {
                                        if (cipherListView.model.get(i).name === cipherDropDown.text) {
                                            currentIndex = i
                                        }
                                    }
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.topMargin: 32
                            Layout.preferredHeight: checkboxLayout.implicitHeight
                            color: "#1C1D21"
                            radius: 16

                            ColumnLayout {
                                id: checkboxLayout

                                anchors.fill: parent
                                CheckBoxType {
                                    Layout.fillWidth: true

                                    text: qsTr("TLS auth")
                                    checked: tlsAuth

                                    onCheckedChanged: {
                                        if (checked !== tlsAuth) {
                                            tlsAuth = checked
                                        }
                                    }
                                }

                                DividerType {}

                                CheckBoxType {
                                    Layout.fillWidth: true

                                    text: qsTr("Block DNS requests outside of VPN")
                                    checked: blockDns

                                    onCheckedChanged: {
                                        if (checked !== blockDns) {
                                            blockDns = checked
                                        }
                                    }
                                }
                            }
                        }

                        SwitcherType {
                            id: additionalClientCommandsSwitcher
                            Layout.fillWidth: true
                            Layout.topMargin: 32

                            checked: additionalClientCommands !== ""

                            text: qsTr("Additional client configuration commands")

                            onCheckedChanged: {
                                if (!checked) {
                                    additionalClientCommands = ""
                                }
                            }
                        }

                        TextAreaType {
                            Layout.fillWidth: true
                            Layout.topMargin: 16

                            visible: additionalClientCommandsSwitcher.checked

                            textAreaText: additionalClientCommands
                            placeholderText: qsTr("Commands:")

                            textArea.onEditingFinished: {
                                if (additionalClientCommands !== textAreaText) {
                                    additionalClientCommands = textAreaText
                                }
                            }
                        }

                        SwitcherType {
                            id: additionalServerCommandsSwitcher
                            Layout.fillWidth: true
                            Layout.topMargin: 16

                            checked: additionalServerCommands !== ""

                            text: qsTr("Additional server configuration commands")

                            onCheckedChanged: {
                                if (!checked) {
                                    additionalServerCommands = ""
                                }
                            }
                        }

                        TextAreaType {
                            Layout.fillWidth: true
                            Layout.topMargin: 16

                            visible: additionalServerCommandsSwitcher.checked

                            textAreaText: additionalServerCommands
                            placeholderText: qsTr("Commands:")

                            textArea.onEditingFinished: {
                                if (additionalServerCommands !== textAreaText) {
                                    additionalServerCommands = textAreaText
                                }
                            }
                        }

                        BasicButtonType {
                            Layout.topMargin: 24
                            Layout.leftMargin: -8
                            implicitHeight: 32

                            visible: ContainersModel.getCurrentlyProcessedContainerIndex() === ContainerEnum.OpenVpn

                            defaultColor: "transparent"
                            hoveredColor: Qt.rgba(1, 1, 1, 0.08)
                            pressedColor: Qt.rgba(1, 1, 1, 0.12)
                            textColor: "#EB5757"

                            text: qsTr("Remove OpenVPN")

                            clickedFunc: function() {
                                var headerText = qsTr("Remove OpenVpn from server?")
                                var descriptionText = qsTr("All users with whom you shared a connection will no longer be able to connect to it.")
                                var yesButtonText = qsTr("Continue")
                                var noButtonText = qsTr("Cancel")

                                var yesButtonFunction = function() {
                                    PageController.goToPage(PageEnum.PageDeinstalling)
                                    InstallController.removeCurrentlyProcessedContainer()
                                }
                                var noButtonFunction = function() {
                                }

                                showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                            }
                        }

                        BasicButtonType {
                            id: saveRestartButton

                            Layout.fillWidth: true
                            Layout.topMargin: 24
                            Layout.bottomMargin: 24

                            text: qsTr("Save and Restart Amnezia")

                            clickedFunc: function() {
                                forceActiveFocus()
                                PageController.goToPage(PageEnum.PageSetupWizardInstalling);
                                InstallController.updateContainer(OpenVpnConfigModel.getConfig())
                            }
                        }
                    }
                }
            }
        }
    }
}
