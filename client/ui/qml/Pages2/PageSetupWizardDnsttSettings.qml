import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"

PageType {
    id: root

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20 + PageController.safeAreaTopMargin

        onFocusChanged: {
            if (this.activeFocus) {
                listView.positionViewAtBeginning()
            }
        }
    }

    QtObject {
        id: transportHint

        // Presentation-only classification of the first resolver. All
        // validation lives in DnsttConfigModel.
        readonly property string firstResolver: {
            var parts = DnsttConfigModel.resolvers.split(",")
            return parts.length > 0 ? parts[0].trim().toLowerCase() : ""
        }
        readonly property bool isDoh: firstResolver.indexOf("https://") === 0
        readonly property bool isDot: firstResolver.indexOf("dot://") === 0 || firstResolver.indexOf("tls://") === 0
        readonly property bool isPlainUdp: firstResolver.length > 0 && !isDoh && !isDot
    }

    ListViewType {
        id: listView

        anchors.top: backButton.bottom
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.left: parent.left

        header: ColumnLayout {
            width: listView.width
            spacing: 8

            BaseHeaderType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                headerText: qsTr("DNSTT Connection")
                descriptionText: qsTr("DNS tunnel with Noise NK encryption and DoH/DoT transport")
            }
        }

        model: 1
        spacing: 16

        delegate: ColumnLayout {
            width: listView.width
            spacing: 16

            TextFieldWithHeaderType {
                id: domainField
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                headerText: qsTr("Tunnel domain") +
                            (DnsttConfigModel.domain.length > 0
                                 ? (" (MTU: " + DnsttConfigModel.calculatedMtu + qsTr(" bytes") + ")")
                                 : "")
                textField.placeholderText: "t.example.com"
                textField.text: DnsttConfigModel.domain
                textField.onTextChanged: DnsttConfigModel.domain = textField.text

                buttonText: qsTr("Paste")
                clickedFunc: function() {
                    textField.text = ""
                    textField.paste()
                    DnsttConfigModel.domain = textField.text
                }
            }

            WarningType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                visible: DnsttConfigModel.domain.length > 0 && !DnsttConfigModel.isMtuValid
                textString: qsTr("Domain is too long! dnstt requires an MTU of at least 80 bytes (current MTU: %1 bytes)")
                        .arg(DnsttConfigModel.calculatedMtu)
            }

            TextFieldWithHeaderType {
                id: resolversField
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                headerText: qsTr("DNS Resolvers (comma separated)")
                textField.placeholderText: "https://1.1.1.1/dns-query"
                textField.text: DnsttConfigModel.resolvers
                textField.onTextChanged: DnsttConfigModel.resolvers = textField.text
            }

            WarningType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                visible: transportHint.isPlainUdp
                textString: qsTr("UDP mode transmits plain DNS packets and is vulnerable to DPI detection. Prefer DoH (https://) or DoT (dot://).")
            }

            TextFieldWithHeaderType {
                id: bootstrapField
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                visible: DnsttConfigModel.needsBootstrap
                headerText: qsTr("Bootstrap DNS IP")
                textField.placeholderText: "1.1.1.1"
                textField.text: DnsttConfigModel.bootstrapIp
                textField.onTextChanged: DnsttConfigModel.bootstrapIp = textField.text
            }

            TextFieldWithHeaderType {
                id: keyField
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                headerText: qsTr("Server Public Key (64 hex characters)")
                textField.placeholderText: "0123456789abcdef..."
                textField.text: DnsttConfigModel.publicKey
                textField.onTextChanged: DnsttConfigModel.publicKey = textField.text

                buttonText: qsTr("Paste")
                clickedFunc: function() {
                    textField.text = ""
                    textField.paste()
                    DnsttConfigModel.publicKey = textField.text
                }
            }

            WarningType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                visible: DnsttConfigModel.publicKey.length > 0 && !DnsttConfigModel.isPublicKeyValid
                textString: qsTr("The public key must be exactly 64 hexadecimal characters.")
            }

            CardType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                headerText: qsTr("Protocol specifics")
                bodyText: qsTr("• TCP-only: UDP traffic (QUIC, VoIP, games) is not carried; only DNS is relayed, over TCP.\n• VPS setup: dnstt-server must forward streams to a local SOCKS5 proxy.\n• Speed: typical throughput is 100-500 Kbps.")
            }
        }

        footer: ColumnLayout {
            width: listView.width
            Layout.topMargin: 16
            Layout.bottomMargin: 32

            BasicButtonType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                text: qsTr("Continue")
                enabled: DnsttConfigModel.isValid

                clickedFunc: function() {
                    if (ImportController.extractConfigFromData(DnsttConfigModel.generateUri())) {
                        PageController.goToPage(PageEnum.PageSetupWizardViewConfig)
                    }
                }
            }
        }
    }
}
