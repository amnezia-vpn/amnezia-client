import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QtCore

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

    property bool isEnabled: ServersUiController.isProcessedServerHasWriteAccess()

    readonly property bool isSaveButtonEnabled: underloadPacketMagicHeaderTextField.errorText === "" &&
                                                transportPacketMagicHeaderTextField.errorText === "" &&
                                                responsePacketMagicHeaderTextField.errorText === "" &&
                                                initPacketMagicHeaderTextField.errorText === "" &&
                                                contentPaddingAdditionTextField.errorText === "" &&
                                                rekeyAfterTimeTextField.errorText === "" &&
                                                rekeyTimeoutTextField.errorText === "" &&
                                                rejectAfterTimeTextField.errorText === "" &&
                                                keepaliveTimeoutTextField.errorText === "" &&
                                                maxHandshakeAttemptsTextField.errorText === "" &&
                                                responsePacketJunkSizeTextField.errorText === "" &&
                                                cookieReplyPacketJunkSizeTextField.errorText === "" &&
                                                transportPacketJunkSizeTextField.errorText === "" &&
                                                initPacketJunkSizeTextField.errorText === "" &&
                                                junkPacketMaxSizeTextField.errorText === "" &&
                                                junkPacketMinSizeTextField.errorText === "" &&
                                                junkPacketCountTextField.errorText === "" &&
                                                portTextField.errorText === "" &&
                                                vpnAddressSubnetTextField.errorText === ""

    function save() {
        if (root.isEnabled) {
            if (AwgConfigModel.isHeadersEqual(underloadPacketMagicHeaderTextField.textField.text,
                                              transportPacketMagicHeaderTextField.textField.text,
                                              responsePacketMagicHeaderTextField.textField.text,
                                              initPacketMagicHeaderTextField.textField.text)) {
                PageController.showErrorMessage(qsTr("The values of the H1-H4 fields must be unique"))
                return
            }

            if (AwgConfigModel.isPacketSizeEqual(parseInt(initPacketJunkSizeTextField.textField.text) || 0,
                                                 parseInt(responsePacketJunkSizeTextField.textField.text) || 0,
                                                 parseInt(cookieReplyPacketJunkSizeTextField.textField.text) || 0,
                                                 parseInt(transportPacketJunkSizeTextField.textField.text) || 0)) {
                PageController.showErrorMessage(qsTr("The value of the field S1 + message initiation size (148) must not equal S2 + message response size (92) + S3 + cookie reply size (64) + S4 + transport packet size (32)"))
                return
            }
        }

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
            InstallController.updateServerConfig(ServersUiController.processedServerId, ServersUiController.processedContainerIndex, ProtocolEnum.Awg)
        }

        var noButtonFunction = function() {}

        Dialogs.askQuestion(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
    }

    SmartScroll {
        id: smartScroll
        flickable: flickable
    }

    FlickableType {
        id: flickable

        anchors.top: parent.top
        anchors.bottom: saveRestartButton.top
        anchors.bottomMargin: 24

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

                title: qsTr("AmneziaWG settings")
            }

            TextFieldType {
                id: vpnAddressSubnetTextField

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                enabled: root.isEnabled

                headerText: qsTr("VPN address subnet")
                textField.text: AwgConfigModel.subnetAddress

                textField.onTextEdited: AwgConfigModel.subnetAddress = textField.text

                textField.onActiveFocusChanged: {
                    if (textField.activeFocus) {
                        smartScroll.scrollToItem(vpnAddressSubnetTextField)
                    }
                }

                checkEmptyText: true
            }

            TextFieldType {
                id: portTextField

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                enabled: root.isEnabled

                headerText: qsTr("Port")
                textField.text: AwgConfigModel.port
                textField.maximumLength: 5
                textField.validator: IntValidator { bottom: 1; top: 65535 }

                textField.onTextEdited: AwgConfigModel.port = textField.text

                textField.onActiveFocusChanged: {
                    if (textField.activeFocus) {
                        smartScroll.scrollToItem(portTextField)
                    }
                }

                checkEmptyText: true
            }

            AwgTextField {
                id: junkPacketCountTextField

                headerText: qsTr("Jc - Junk packet count")
                textField.text: AwgConfigModel.serverJunkPacketCount

                scroller: smartScroll
                onEdited: (text) => { AwgConfigModel.serverJunkPacketCount = text }
            }

            AwgTextField {
                id: junkPacketMinSizeTextField

                headerText: qsTr("Jmin - Junk packet minimum size")
                textField.text: AwgConfigModel.serverJunkPacketMinSize

                scroller: smartScroll
                onEdited: (text) => { AwgConfigModel.serverJunkPacketMinSize = text }
            }

            AwgTextField {
                id: junkPacketMaxSizeTextField

                headerText: qsTr("Jmax - Junk packet maximum size")
                textField.text: AwgConfigModel.serverJunkPacketMaxSize

                scroller: smartScroll
                onEdited: (text) => { AwgConfigModel.serverJunkPacketMaxSize = text }
            }

            AwgTextField {
                id: initPacketJunkSizeTextField

                headerText: qsTr("S1 - Init packet junk size")
                textField.text: AwgConfigModel.serverInitPacketJunkSize

                scroller: smartScroll
                onEdited: (text) => { AwgConfigModel.serverInitPacketJunkSize = text }
            }

            AwgTextField {
                id: responsePacketJunkSizeTextField

                headerText: qsTr("S2 - Response packet junk size")
                textField.text: AwgConfigModel.serverResponsePacketJunkSize

                scroller: smartScroll
                onEdited: (text) => { AwgConfigModel.serverResponsePacketJunkSize = text }
            }

            AwgTextField {
                id: cookieReplyPacketJunkSizeTextField

                visible: AwgConfigModel.isAwg2

                headerText: qsTr("S3 - Cookie reply packet junk size")
                textField.text: AwgConfigModel.serverCookieReplyPacketJunkSize

                scroller: smartScroll
                onEdited: (text) => { AwgConfigModel.serverCookieReplyPacketJunkSize = text }
            }

            AwgTextField {
                id: transportPacketJunkSizeTextField

                visible: AwgConfigModel.isAwg2

                headerText: qsTr("S4 - Transport packet junk size")
                textField.text: AwgConfigModel.serverTransportPacketJunkSize

                scroller: smartScroll
                onEdited: (text) => { AwgConfigModel.serverTransportPacketJunkSize = text }
            }

            AwgTextField {
                id: initPacketMagicHeaderTextField

                rangeValidation: true

                headerText: qsTr("H1 - Init packet magic header")
                textField.text: AwgConfigModel.serverInitPacketMagicHeader

                scroller: smartScroll
                onEdited: (text) => { AwgConfigModel.serverInitPacketMagicHeader = text }
            }

            AwgTextField {
                id: responsePacketMagicHeaderTextField

                rangeValidation: true

                headerText: qsTr("H2 - Response packet magic header")
                textField.text: AwgConfigModel.serverResponsePacketMagicHeader

                scroller: smartScroll
                onEdited: (text) => { AwgConfigModel.serverResponsePacketMagicHeader = text }
            }

            AwgTextField {
                id: underloadPacketMagicHeaderTextField

                rangeValidation: true

                headerText: qsTr("H3 - Underload packet magic header")
                textField.text: AwgConfigModel.serverUnderloadPacketMagicHeader

                scroller: smartScroll
                onEdited: (text) => { AwgConfigModel.serverUnderloadPacketMagicHeader = text }
            }

            AwgTextField {
                id: transportPacketMagicHeaderTextField

                rangeValidation: true

                headerText: qsTr("H4 - Transport packet magic header")
                textField.text: AwgConfigModel.serverTransportPacketMagicHeader

                scroller: smartScroll
                onEdited: (text) => { AwgConfigModel.serverTransportPacketMagicHeader = text }
            }

            AwgTextField {
                id: specialJunk1TextField

                headerText: qsTr("I1 - Special junk 1")
                textField.text: AwgConfigModel.serverSpecialJunk1

                scroller: smartScroll
                onEdited: (text) => { AwgConfigModel.serverSpecialJunk1 = text }
            }

            AwgTextField {
                id: specialJunk2TextField

                headerText: qsTr("I2 - Special junk 2")
                textField.text: AwgConfigModel.serverSpecialJunk2

                scroller: smartScroll
                onEdited: (text) => { AwgConfigModel.serverSpecialJunk2 = text }
            }

            AwgTextField {
                id: specialJunk3TextField

                headerText: qsTr("I3 - Special junk 3")
                textField.text: AwgConfigModel.serverSpecialJunk3

                scroller: smartScroll
                onEdited: (text) => { AwgConfigModel.serverSpecialJunk3 = text }
            }

            AwgTextField {
                id: specialJunk4TextField

                headerText: qsTr("I4 - Special junk 4")
                textField.text: AwgConfigModel.serverSpecialJunk4

                scroller: smartScroll
                onEdited: (text) => { AwgConfigModel.serverSpecialJunk4 = text }
            }

            AwgTextField {
                id: specialJunk5TextField

                headerText: qsTr("I5 - Special junk 5")
                textField.text: AwgConfigModel.serverSpecialJunk5

                scroller: smartScroll
                onEdited: (text) => { AwgConfigModel.serverSpecialJunk5 = text }
            }

            CheckBoxType {
                id: headerProtectionCheckBox

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                visible: AwgConfigModel.isAwg3

                text: qsTr("HeaderProtectionKey")

                checked: AwgConfigModel.serverHeaderProtectionEnabled
                onCheckedChanged: {
                    if (checked !== AwgConfigModel.serverHeaderProtectionEnabled) {
                        AwgConfigModel.serverHeaderProtectionEnabled = checked
                    }
                }
            }

            AwgTextField {
                id: contentPaddingAdditionTextField

                visible: AwgConfigModel.isAwg3
                rangeValidation: true

                headerText: qsTr("ContentPaddingAddition - Content padding addition")
                textField.text: AwgConfigModel.serverContentPaddingAddition

                scroller: smartScroll
                onEdited: (text) => { AwgConfigModel.serverContentPaddingAddition = text }
            }

            AwgTextField {
                id: rekeyAfterTimeTextField

                visible: AwgConfigModel.isAwg3
                rangeValidation: true

                headerText: qsTr("RekeyAfterTime - Rekey after time")
                textField.text: AwgConfigModel.serverRekeyAfterTime

                scroller: smartScroll
                onEdited: (text) => { AwgConfigModel.serverRekeyAfterTime = text }
            }

            AwgTextField {
                id: rekeyTimeoutTextField

                visible: AwgConfigModel.isAwg3
                rangeValidation: true

                headerText: qsTr("RekeyTimeout - Rekey timeout")
                textField.text: AwgConfigModel.serverRekeyTimeout

                scroller: smartScroll
                onEdited: (text) => { AwgConfigModel.serverRekeyTimeout = text }
            }

            AwgTextField {
                id: rejectAfterTimeTextField

                visible: AwgConfigModel.isAwg3
                rangeValidation: true

                headerText: qsTr("RejectAfterTime - Reject after time")
                textField.text: AwgConfigModel.serverRejectAfterTime

                scroller: smartScroll
                onEdited: (text) => { AwgConfigModel.serverRejectAfterTime = text }
            }

            AwgTextField {
                id: keepaliveTimeoutTextField

                visible: AwgConfigModel.isAwg3
                rangeValidation: true

                headerText: qsTr("KeepaliveTimeout - Keepalive timeout")
                textField.text: AwgConfigModel.serverKeepaliveTimeout

                scroller: smartScroll
                onEdited: (text) => { AwgConfigModel.serverKeepaliveTimeout = text }
            }

            AwgTextField {
                id: maxHandshakeAttemptsTextField

                visible: AwgConfigModel.isAwg3
                rangeValidation: true

                headerText: qsTr("MaxHandshakeAttempts - Max handshake attempts")
                textField.text: AwgConfigModel.serverMaxHandshakeAttempts

                scroller: smartScroll
                onEdited: (text) => { AwgConfigModel.serverMaxHandshakeAttempts = text }
            }

            CheckBoxType {
                id: randomTrailersCheckBox

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                visible: AwgConfigModel.isAwg3

                text: qsTr("RandomTrailers")

                checked: AwgConfigModel.serverRandomTrailers
                onCheckedChanged: {
                    if (checked !== AwgConfigModel.serverRandomTrailers) {
                        AwgConfigModel.serverRandomTrailers = checked
                    }
                }
            }

            CheckBoxType {
                id: disableCookiesCheckBox

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                visible: AwgConfigModel.isAwg3

                text: qsTr("DisableCookies")

                checked: AwgConfigModel.serverDisableCookies
                onCheckedChanged: {
                    if (checked !== AwgConfigModel.serverDisableCookies) {
                        AwgConfigModel.serverDisableCookies = checked
                    }
                }
            }
        }
    }

    ButtonType {
        id: saveRestartButton

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        anchors.bottomMargin: 8 + PageController.safeAreaBottomMargin

        enabled: root.isSaveButtonEnabled

        text: qsTr("Save")

        onClicked: {
            root.save()
        }
    }
}
