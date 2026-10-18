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

    property bool editDirty: false

    ListViewType {
        id: listView

        anchors.top: parent.top
        anchors.bottom: saveButton.visible ? saveButton.top : parent.bottom
        anchors.bottomMargin: saveButton.visible ? 16 : 0
        anchors.left: parent.left
        anchors.right: parent.right

        enabled: ServersUiController.isProcessedServerHasWriteAccess()
        model: XrayConfigModel

        header: NavigationBarType {
            width: listView.width

            title: qsTr("Security")
        }

        delegate: ColumnLayout {
            width: listView.width
            spacing: 0

            readonly property bool realityAllowed: transport !== "mkcp"

            RadioItemType {
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                text: qsTr("None")
                checked: security === "none" || (security === "reality" && !realityAllowed)
                onClicked: security = "none"
            }

            RadioItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                text: qsTr("TLS")
                checked: security === "tls"
                onClicked: security = "tls"
            }

            RadioItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                text: qsTr("Reality")
                enabled: realityAllowed
                checked: security === "reality" && realityAllowed
                onClicked: security = "reality"
            }

            SmallTextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 12
                visible: !realityAllowed
                color: AmneziaStyle.color.textAccent
                text: qsTr("REALITY is not supported with the mKCP transport. Use None or TLS.")
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                visible: security === "tls"
                spacing: 12

                DropDownType {
                    id: tlsAlpnDropDown
                    fitContent: true
                    Layout.fillWidth: true
                    text: alpn
                    descriptionText: qsTr("ALPN")
                    headerText: qsTr("ALPN")
                    listView: RadioListType {
                        currentValue: alpn
                        model: ListModel {
                            Component.onCompleted: {
                                var opts = XrayConfigModel.alpnOptions()
                                for (var i = 0; i < opts.length; i++) {
                                    append({name: opts[i]})
                                }
                            }
                        }
                        onActivated: {
                            alpn = selectedText
                            tlsAlpnDropDown.text = selectedText
                            tlsAlpnDropDown.close()
                        }
                        Component.onCompleted: {
                            for (var i = 0; i < model.count; i++) {
                                if (model.get(i).name === alpn) {
                                    selectedIndex = i;
                                    break
                                }
                            }
                        }
                    }
                    Connections {
                        target: XrayConfigModel

                        function onDataChanged() {
                            tlsAlpnDropDown.text = alpn
                        }
                    }
                }

                DropDownType {
                    id: tlsFingerprintDropDown
                    fitContent: true
                    Layout.fillWidth: true
                    text: fingerprint
                    descriptionText: qsTr("Fingerprint")
                    headerText: qsTr("Fingerprint")
                    listView: RadioListType {
                        model: ListModel {
                            Component.onCompleted: {
                                var opts = XrayConfigModel.fingerprintOptions()
                                for (var i = 0; i < opts.length; i++) {
                                    append({name: opts[i]})
                                }
                            }
                        }
                        currentValue: fingerprint
                        onActivated: {
                            fingerprint = selectedText
                            tlsFingerprintDropDown.text = selectedText
                            tlsFingerprintDropDown.close()
                        }
                        Component.onCompleted: {
                            for (var i = 0; i < model.count; i++) {
                                if (model.get(i).name === fingerprint) {
                                    selectedIndex = i;
                                    break
                                }
                            }
                        }
                    }
                    Connections {
                        target: XrayConfigModel

                        function onDataChanged() {
                            tlsFingerprintDropDown.text = fingerprint
                        }
                    }
                }

                TextFieldType {
                    id: sniFieldTls
                    Layout.fillWidth: true
                    headerText: qsTr("Server Name (SNI)")
                    placeholderText: XrayConfigModel.sniDefault()
                    textField.text: sni
                    textField.validator: RegularExpressionValidator { regularExpression: /^[A-Za-z0-9.*_-]*$/ }
                    textField.onTextEdited: root.editDirty = (textField.text !== sni)
                    textField.onEditingFinished: {
                        var v = textField.text.trim()
                        if (v !== sni) sni = v
                        else if (textField.text !== v) textField.text = v
                        sniFieldTls.errorText = XrayConfigModel.isValidSni(v) ? "" : qsTr("Enter a valid IP address or domain name")
                        root.editDirty = false
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                visible: security === "reality" && realityAllowed
                spacing: 12

                DropDownType {
                    id: realityFingerprintDropDown
                    fitContent: true
                    Layout.fillWidth: true
                    text: fingerprint
                    descriptionText: qsTr("Fingerprint")
                    headerText: qsTr("Fingerprint")
                    listView: RadioListType {
                        model: ListModel {
                            Component.onCompleted: {
                                var opts = XrayConfigModel.fingerprintOptions()
                                for (var i = 0; i < opts.length; i++) {
                                    append({name: opts[i]})
                                }
                            }
                        }
                        currentValue: fingerprint
                        onActivated: {
                            fingerprint = selectedText
                            realityFingerprintDropDown.text = selectedText
                            realityFingerprintDropDown.close()
                        }
                        Component.onCompleted: {
                            for (var i = 0; i < model.count; i++) {
                                if (model.get(i).name === fingerprint) {
                                    selectedIndex = i;
                                    break
                                }
                            }
                        }
                    }
                    Connections {
                        target: XrayConfigModel

                        function onDataChanged() {
                            realityFingerprintDropDown.text = fingerprint
                        }
                    }
                }

                TextFieldType {
                    id: sniFieldReality
                    Layout.fillWidth: true
                    headerText: qsTr("Server Name (SNI)")
                    placeholderText: XrayConfigModel.sniDefault()
                    textField.text: sni
                    textField.validator: RegularExpressionValidator { regularExpression: /^[A-Za-z0-9.*_-]*$/ }
                    textField.onTextEdited: root.editDirty = (textField.text !== sni)
                    textField.onEditingFinished: {
                        var v = textField.text.trim()
                        if (v !== sni) sni = v
                        else if (textField.text !== v) textField.text = v
                        sniFieldReality.errorText = XrayConfigModel.isValidSni(v) ? "" : qsTr("Enter a valid IP address or domain name")
                        root.editDirty = false
                    }
                }
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

        visible: listView.enabled && (XrayConfigModel.hasUnsavedChanges || root.editDirty)
        enabled: visible
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
                PageController.goToPage(PageEnum.PageSetupWizardInstalling)
                InstallController.updateServerConfig(ServersUiController.processedServerId, ServersUiController.processedContainerIndex, ProtocolEnum.Xray)
            }
            var noButtonFunction = function () {}
            showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
        }
    }
}
