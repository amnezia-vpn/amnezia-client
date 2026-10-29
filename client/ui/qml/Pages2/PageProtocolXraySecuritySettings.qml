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

    readonly property bool realityAllowed: XrayConfigModel.transport !== "mkcp"

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

                title: qsTr("Security")
            }

            RadioItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                text: qsTr("None")
                checked: XrayConfigModel.security === "none" || (XrayConfigModel.security === "reality" && !root.realityAllowed)
                onClicked: XrayConfigModel.security = "none"
            }

            RadioItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                text: qsTr("TLS")
                checked: XrayConfigModel.security === "tls"
                onClicked: XrayConfigModel.security = "tls"
            }

            RadioItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                text: qsTr("Reality")
                enabled: root.realityAllowed
                checked: XrayConfigModel.security === "reality" && root.realityAllowed
                onClicked: XrayConfigModel.security = "reality"
            }

            SmallTextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 12
                visible: !root.realityAllowed
                color: AmneziaStyle.color.textAccent
                text: qsTr("REALITY is not supported with the mKCP transport. Use None or TLS.")
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                visible: XrayConfigModel.security === "tls"
                spacing: 12

                DropDownType {
                    id: tlsAlpnDropDown
                    fitContent: true
                    Layout.fillWidth: true
                    text: XrayConfigModel.alpn
                    descriptionText: qsTr("ALPN")
                    headerText: qsTr("ALPN")
                    listView: RadioListType {
                        currentValue: XrayConfigModel.alpn
                        model: ListModel {
                            Component.onCompleted: {
                                var opts = XrayConfigModel.alpnOptions()
                                for (var i = 0; i < opts.length; i++) {
                                    append({name: opts[i]})
                                }
                            }
                        }
                        onActivated: {
                            XrayConfigModel.alpn = selectedText
                            tlsAlpnDropDown.close()
                        }
                        Component.onCompleted: {
                            for (var i = 0; i < model.count; i++) {
                                if (model.get(i).name === XrayConfigModel.alpn) {
                                    selectedIndex = i;
                                    break
                                }
                            }
                        }
                    }
                }

                DropDownType {
                    id: tlsFingerprintDropDown
                    visible: XrayConfigModel.transport !== "mkcp"
                    fitContent: true
                    Layout.fillWidth: true
                    text: XrayConfigModel.fingerprint
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
                        currentValue: XrayConfigModel.fingerprint
                        onActivated: {
                            XrayConfigModel.fingerprint = selectedText
                            tlsFingerprintDropDown.close()
                        }
                        Component.onCompleted: {
                            for (var i = 0; i < model.count; i++) {
                                if (model.get(i).name === XrayConfigModel.fingerprint) {
                                    selectedIndex = i;
                                    break
                                }
                            }
                        }
                    }
                }

                TextFieldType {
                    id: sniFieldTls
                    Layout.fillWidth: true
                    headerText: qsTr("Server Name (SNI)")
                    placeholderText: XrayConfigModel.sniDefault()
                    textField.text: XrayConfigModel.sni
                    textField.validator: RegularExpressionValidator { regularExpression: /^[A-Za-z0-9._-]*$/ }
                    textField.onTextEdited: XrayConfigModel.sni = textField.text
                    textField.onEditingFinished: {
                        var v = textField.text.trim()
                        if (v !== XrayConfigModel.sni) XrayConfigModel.sni = v
                        sniFieldTls.errorText = XrayConfigModel.isValidSni(v) ? "" : qsTr("Enter a valid IP address or domain name")
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                visible: XrayConfigModel.security === "reality" && root.realityAllowed
                spacing: 12

                DropDownType {
                    id: realityFingerprintDropDown
                    fitContent: true
                    Layout.fillWidth: true
                    text: XrayConfigModel.fingerprint
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
                        currentValue: XrayConfigModel.fingerprint
                        onActivated: {
                            XrayConfigModel.fingerprint = selectedText
                            realityFingerprintDropDown.close()
                        }
                        Component.onCompleted: {
                            for (var i = 0; i < model.count; i++) {
                                if (model.get(i).name === XrayConfigModel.fingerprint) {
                                    selectedIndex = i;
                                    break
                                }
                            }
                        }
                    }
                }

                TextFieldType {
                    id: sniFieldReality
                    Layout.fillWidth: true
                    headerText: qsTr("Server Name (SNI)")
                    placeholderText: XrayConfigModel.sniDefault()
                    textField.text: XrayConfigModel.sni
                    textField.validator: RegularExpressionValidator { regularExpression: /^[A-Za-z0-9._-]*$/ }
                    textField.onTextEdited: XrayConfigModel.sni = textField.text
                    textField.onEditingFinished: {
                        var v = textField.text.trim()
                        if (v !== XrayConfigModel.sni) XrayConfigModel.sni = v
                        sniFieldReality.errorText = XrayConfigModel.isValidSni(v) ? "" : qsTr("Enter a valid IP address or domain name")
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
