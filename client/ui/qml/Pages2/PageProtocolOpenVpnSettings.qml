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

    ColumnLayout {
        id: backButtonLayout

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        anchors.topMargin: 20

        BackButtonType {
            id: backButton
        }
    }

    FlickableType {
        id: fl
        anchors.top: backButtonLayout.bottom
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

                            parentFlickable: fl

                            textField.onEditingFinished: {
                                if (textFieldText !== subnetAddress) {
                                    subnetAddress = textFieldText
                                }
                            }
                        }

                        ParagraphTextType {
                            Layout.fillWidth: true
                            Layout.topMargin: 32

                            text: qsTr("Network protocol")
                        }

                        TransportProtoSelector {
                            id: transportProtoSelector
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
                            parentFlickable: fl

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
                        }

                        SwitcherType {
                            id: autoNegotiateEncryprionSwitcher

                            Layout.fillWidth: true
                            Layout.topMargin: 24
                            parentFlickable: fl

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
                            parentFlickable: fl

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
                            parentFlickable: fl

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
                            id: contentRect
                            Layout.fillWidth: true
                            Layout.topMargin: 32
                            Layout.preferredHeight: checkboxLayout.implicitHeight
                            color: AmneziaStyle.color.onyxBlack
                            radius: 16

                            Connections {
                                target: tlsAuthCheckBox
                                enabled: !GC.isMobile()

                                function onFocusChanged() {
                                    if (tlsAuthCheckBox.activeFocus) {
                                        fl.ensureVisible(contentRect)
                                    }
                                }
                            }

                            ColumnLayout {
                                id: checkboxLayout

                                anchors.fill: parent
                                CheckBoxType {
                                    id: tlsAuthCheckBox
                                    Layout.fillWidth: true

                                    text: qsTr("TLS auth")
                                    checked: tlsAuth

                                    onCheckedChanged: {
                                        if (checked !== tlsAuth) {
                                            console.log("tlsAuth changed to: " + checked)
                                            tlsAuth = checked
                                        }
                                    }
                                }

                                DividerType {}

                                CheckBoxType {
                                    id: blockDnsCheckBox
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
                            parentFlickable: fl

                            checked: additionalClientCommands !== ""

                            text: qsTr("Additional client configuration commands")

                            onCheckedChanged: {
                                if (!checked) {
                                    additionalClientCommands = ""
                                }
                            }
                        }

                        TextAreaType {
                            id: additionalClientCommandsTextArea
                            Layout.fillWidth: true
                            Layout.topMargin: 16

                            visible: additionalClientCommandsSwitcher.checked

                            parentFlickable: fl

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
                            parentFlickable: fl

                            checked: additionalServerCommands !== ""

                            text: qsTr("Additional server configuration commands")

                            onCheckedChanged: {
                                if (!checked) {
                                    additionalServerCommands = ""
                                }
                            }
                        }

                        TextAreaType {
                            id: additionalServerCommandsTextArea
                            Layout.fillWidth: true
                            Layout.topMargin: 16

                            visible: additionalServerCommandsSwitcher.checked

                            textAreaText: additionalServerCommands
                            placeholderText: qsTr("Commands:")
                            parentFlickable: fl
                            textArea.onEditingFinished: {
                                if (additionalServerCommands !== textAreaText) {
                                    additionalServerCommands = textAreaText
                                }
                            }
                        }

                        BasicButtonType {
                            id: saveRestartButton

                            Layout.fillWidth: true
                            Layout.topMargin: 24
                            Layout.bottomMargin: 24

                            text: qsTr("Save")
                            parentFlickable: fl
                            Keys.onTabPressed: lastItemTabClicked(focusItem)

                            clickedFunc: function() {
                                forceActiveFocus()

                                if (ConnectionController.isConnected && ServersModel.getDefaultServerData("defaultContainer") === ContainersModel.getProcessedContainerIndex()) {
                                    PageController.showNotificationMessage(qsTr("Unable change settings while there is an active connection"))
                                    return
                                }

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
