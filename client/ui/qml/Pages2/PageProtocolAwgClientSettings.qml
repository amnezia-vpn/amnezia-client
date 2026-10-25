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

    readonly property bool isSaveButtonEnabled: mtuTextField.errorText === "" &&
                                                junkPacketMaxSizeTextField.errorText === "" &&
                                                junkPacketMinSizeTextField.errorText === "" &&
                                                junkPacketCountTextField.errorText === ""

    SmartScroll {
        id: smartScroll
        flickable: flickable
    }

    FlickableType {
        id: flickable

        anchors.top: parent.top
        anchors.bottom: saveButton.top
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
                id: mtuTextField

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("MTU")
                textField.text: AwgConfigModel.clientMtu
                textField.validator: IntValidator { bottom: 576; top: 65535 }

                textField.onTextEdited: AwgConfigModel.clientMtu = textField.text

                textField.onActiveFocusChanged: {
                    if (textField.activeFocus) {
                        smartScroll.scrollToItem(mtuTextField)
                    }
                }

                checkEmptyText: true
            }

            AwgTextField {
                id: junkPacketCountTextField

                headerText: "Jc - Junk packet count"
                textField.text: AwgConfigModel.clientJunkPacketCount

                scroller: smartScroll
                onEdited: (text) => { AwgConfigModel.clientJunkPacketCount = text }
            }

            AwgTextField {
                id: junkPacketMinSizeTextField

                headerText: "Jmin - Junk packet minimum size"
                textField.text: AwgConfigModel.clientJunkPacketMinSize

                scroller: smartScroll
                onEdited: (text) => { AwgConfigModel.clientJunkPacketMinSize = text }
            }

            AwgTextField {
                id: junkPacketMaxSizeTextField

                headerText: "Jmax - Junk packet maximum size"
                textField.text: AwgConfigModel.clientJunkPacketMaxSize

                scroller: smartScroll
                onEdited: (text) => { AwgConfigModel.clientJunkPacketMaxSize = text }
            }

            AwgTextField {
                id: specialJunk1TextField

                headerText: qsTr("I1 - First special junk packet")
                textField.text: AwgConfigModel.clientSpecialJunk1

                scroller: smartScroll
                onEdited: (text) => { AwgConfigModel.clientSpecialJunk1 = text }
            }

            AwgTextField {
                id: specialJunk2TextField

                headerText: qsTr("I2 - Second special junk packet")
                textField.text: AwgConfigModel.clientSpecialJunk2

                scroller: smartScroll
                onEdited: (text) => { AwgConfigModel.clientSpecialJunk2 = text }
            }

            AwgTextField {
                id: specialJunk3TextField

                headerText: qsTr("I3 - Third special junk packet")
                textField.text: AwgConfigModel.clientSpecialJunk3

                scroller: smartScroll
                onEdited: (text) => { AwgConfigModel.clientSpecialJunk3 = text }
            }

            AwgTextField {
                id: specialJunk4TextField

                headerText: qsTr("I4 - Fourth special junk packet")
                textField.text: AwgConfigModel.clientSpecialJunk4

                scroller: smartScroll
                onEdited: (text) => { AwgConfigModel.clientSpecialJunk4 = text }
            }

            AwgTextField {
                id: specialJunk5TextField

                headerText: qsTr("I5 - Fifth special junk packet")
                textField.text: AwgConfigModel.clientSpecialJunk5

                scroller: smartScroll
                onEdited: (text) => { AwgConfigModel.clientSpecialJunk5 = text }
            }

            CheckBoxType {
                id: headerProtectionCheckBox

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                enabled: false

                text: qsTr("HeaderProtectionKey")

                checked: AwgConfigModel.clientHeaderProtectionEnabled
            }

            AwgTextField {
                id: contentPaddingAdditionTextField

                rangeValidation: true

                headerText: qsTr("ContentPaddingAddition - Content padding addition")
                textField.text: AwgConfigModel.clientContentPaddingAddition

                scroller: smartScroll
                onEdited: (text) => { AwgConfigModel.clientContentPaddingAddition = text }
            }

            AwgTextField {
                id: rekeyAfterTimeTextField

                rangeValidation: true

                headerText: qsTr("RekeyAfterTime - Rekey after time")
                textField.text: AwgConfigModel.clientRekeyAfterTime

                scroller: smartScroll
                onEdited: (text) => { AwgConfigModel.clientRekeyAfterTime = text }
            }

            AwgTextField {
                id: rekeyTimeoutTextField

                rangeValidation: true

                headerText: qsTr("RekeyTimeout - Rekey timeout")
                textField.text: AwgConfigModel.clientRekeyTimeout

                scroller: smartScroll
                onEdited: (text) => { AwgConfigModel.clientRekeyTimeout = text }
            }

            AwgTextField {
                id: rejectAfterTimeTextField

                rangeValidation: true

                headerText: qsTr("RejectAfterTime - Reject after time")
                textField.text: AwgConfigModel.clientRejectAfterTime

                scroller: smartScroll
                onEdited: (text) => { AwgConfigModel.clientRejectAfterTime = text }
            }

            AwgTextField {
                id: keepaliveTimeoutTextField

                rangeValidation: true

                headerText: qsTr("KeepaliveTimeout - Keepalive timeout")
                textField.text: AwgConfigModel.clientKeepaliveTimeout

                scroller: smartScroll
                onEdited: (text) => { AwgConfigModel.clientKeepaliveTimeout = text }
            }

            AwgTextField {
                id: maxHandshakeAttemptsTextField

                rangeValidation: true

                headerText: qsTr("MaxHandshakeAttempts - Max handshake attempts")
                textField.text: AwgConfigModel.clientMaxHandshakeAttempts

                scroller: smartScroll
                onEdited: (text) => { AwgConfigModel.clientMaxHandshakeAttempts = text }
            }

            CheckBoxType {
                id: randomTrailersCheckBox

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                enabled: false

                text: qsTr("RandomTrailers")

                checked: AwgConfigModel.clientRandomTrailers
            }

            CheckBoxType {
                id: disableCookiesCheckBox

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("DisableCookies")

                checked: AwgConfigModel.clientDisableCookies
                onCheckedChanged: {
                    if (checked !== AwgConfigModel.clientDisableCookies) {
                        AwgConfigModel.clientDisableCookies = checked
                    }
                }
            }

            H2TextType {
                Layout.fillWidth: true
                Layout.topMargin: 28
                Layout.bottomMargin: 12
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Server settings")
            }

            AwgTextField {
                id: portTextField

                enabled: false

                headerText: qsTr("Port")
                textField.text: AwgConfigModel.port
            }

            AwgTextField {
                id: initPacketJunkSizeTextField

                enabled: false

                headerText: "S1 - Init packet junk size"
                textField.text: AwgConfigModel.serverInitPacketJunkSize
            }

            AwgTextField {
                id: responsePacketJunkSizeTextField

                enabled: false

                headerText: "S2 - Response packet junk size"
                textField.text: AwgConfigModel.serverResponsePacketJunkSize
            }

            AwgTextField {
                id: cookieReplyPacketJunkSizeTextField

                visible: AwgConfigModel.isAwg2
                enabled: false

                headerText: "S3 - Cookie Reply packet junk size"
                textField.text: AwgConfigModel.serverCookieReplyPacketJunkSize
            }

            AwgTextField {
                id: transportPacketJunkSizeTextField

                visible: AwgConfigModel.isAwg2
                enabled: false

                headerText: "S4 - Transport packet junk size"
                textField.text: AwgConfigModel.serverTransportPacketJunkSize
            }

            AwgTextField {
                id: initPacketMagicHeaderTextField

                enabled: false

                headerText: "H1 - Init packet magic header"
                textField.text: AwgConfigModel.serverInitPacketMagicHeader
            }

            AwgTextField {
                id: responsePacketMagicHeaderTextField

                enabled: false

                headerText: "H2 - Response packet magic header"
                textField.text: AwgConfigModel.serverResponsePacketMagicHeader
            }

            AwgTextField {
                id: underloadPacketMagicHeaderTextField

                enabled: false

                headerText: "H3 - Underload packet magic header"
                textField.text: AwgConfigModel.serverUnderloadPacketMagicHeader
            }

            AwgTextField {
                id: transportPacketMagicHeaderTextField

                enabled: false

                headerText: "H4 - Transport packet magic header"
                textField.text: AwgConfigModel.serverTransportPacketMagicHeader
            }
        }
    }

    ButtonType {
        id: saveButton

        anchors.right: parent.right
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.rightMargin: 16
        anchors.leftMargin: 16
        anchors.bottomMargin: 8 + PageController.safeAreaBottomMargin

        enabled: root.isSaveButtonEnabled

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

                InstallController.updateClientConfig(ServersUiController.processedServerId, ServersUiController.processedContainerIndex, ProtocolEnum.Awg)
            }

            var noButtonFunction = function() {}

            Dialogs.askQuestion(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
        }
    }
}
