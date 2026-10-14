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

    SmartScroll {
        id: smartScroll
        listView: listView
    }

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

            title: qsTr("AmneziaWG settings")
        }

        model: AwgConfigModel

        delegate: ColumnLayout {
            width: listView.width

            property bool isSaveButtonEnabled: mtuTextField.errorText === "" &&
                                               junkPacketMaxSizeTextField.errorText === "" &&
                                               junkPacketMinSizeTextField.errorText === "" &&
                                               junkPacketCountTextField.errorText === ""

            spacing: 12

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
                textField.text: clientJunkPacketCount

                scroller: smartScroll
                onEdited: (text) => { clientJunkPacketCount = text }
            }

            AwgTextField {
                id: junkPacketMinSizeTextField

                headerText: "Jmin - Junk packet minimum size"
                textField.text: clientJunkPacketMinSize

                scroller: smartScroll
                onEdited: (text) => { clientJunkPacketMinSize = text }
            }

            AwgTextField {
                id: junkPacketMaxSizeTextField

                headerText: "Jmax - Junk packet maximum size"
                textField.text: clientJunkPacketMaxSize

                scroller: smartScroll
                onEdited: (text) => { clientJunkPacketMaxSize = text }
            }

            AwgTextField {
                id: specialJunk1TextField

                headerText: qsTr("I1 - First special junk packet")
                textField.text: clientSpecialJunk1

                scroller: smartScroll
                onEdited: (text) => { clientSpecialJunk1 = text }
            }

            AwgTextField {
                id: specialJunk2TextField

                headerText: qsTr("I2 - Second special junk packet")
                textField.text: clientSpecialJunk2

                scroller: smartScroll
                onEdited: (text) => { clientSpecialJunk2 = text }
            }

            AwgTextField {
                id: specialJunk3TextField

                headerText: qsTr("I3 - Third special junk packet")
                textField.text: clientSpecialJunk3

                scroller: smartScroll
                onEdited: (text) => { clientSpecialJunk3 = text }
            }

            AwgTextField {
                id: specialJunk4TextField

                headerText: qsTr("I4 - Fourth special junk packet")
                textField.text: clientSpecialJunk4

                scroller: smartScroll
                onEdited: (text) => { clientSpecialJunk4 = text }
            }

            AwgTextField {
                id: specialJunk5TextField

                headerText: qsTr("I5 - Fifth special junk packet")
                textField.text: clientSpecialJunk5

                scroller: smartScroll
                onEdited: (text) => { clientSpecialJunk5 = text }
            }

            CheckBoxType {
                id: headerProtectionCheckBox

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                enabled: false

                text: qsTr("HeaderProtectionKey")

                checked: clientHeaderProtectionEnabled
            }

            AwgTextField {
                id: contentPaddingAdditionTextField

                rangeValidation: true

                headerText: qsTr("ContentPaddingAddition - Content padding addition")
                textField.text: clientContentPaddingAddition

                scroller: smartScroll
                onEdited: (text) => { clientContentPaddingAddition = text }
            }

            AwgTextField {
                id: rekeyAfterTimeTextField

                rangeValidation: true

                headerText: qsTr("RekeyAfterTime - Rekey after time")
                textField.text: clientRekeyAfterTime

                scroller: smartScroll
                onEdited: (text) => { clientRekeyAfterTime = text }
            }

            AwgTextField {
                id: rekeyTimeoutTextField

                rangeValidation: true

                headerText: qsTr("RekeyTimeout - Rekey timeout")
                textField.text: clientRekeyTimeout

                scroller: smartScroll
                onEdited: (text) => { clientRekeyTimeout = text }
            }

            AwgTextField {
                id: rejectAfterTimeTextField

                rangeValidation: true

                headerText: qsTr("RejectAfterTime - Reject after time")
                textField.text: clientRejectAfterTime

                scroller: smartScroll
                onEdited: (text) => { clientRejectAfterTime = text }
            }

            AwgTextField {
                id: keepaliveTimeoutTextField

                rangeValidation: true

                headerText: qsTr("KeepaliveTimeout - Keepalive timeout")
                textField.text: clientKeepaliveTimeout

                scroller: smartScroll
                onEdited: (text) => { clientKeepaliveTimeout = text }
            }

            AwgTextField {
                id: maxHandshakeAttemptsTextField

                rangeValidation: true

                headerText: qsTr("MaxHandshakeAttempts - Max handshake attempts")
                textField.text: clientMaxHandshakeAttempts

                scroller: smartScroll
                onEdited: (text) => { clientMaxHandshakeAttempts = text }
            }

            CheckBoxType {
                id: randomTrailersCheckBox

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                enabled: false

                text: qsTr("RandomTrailers")

                checked: clientRandomTrailers
            }

            CheckBoxType {
                id: disableCookiesCheckBox

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("DisableCookies")

                checked: clientDisableCookies
                onCheckedChanged: {
                    if (checked !== clientDisableCookies) {
                        clientDisableCookies = checked
                    }
                }
            }

            H2TextType {
                Layout.fillWidth: true
                Layout.topMargin: 20
                Layout.bottomMargin: 12
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Server settings")
            }

            AwgTextField {
                id: portTextField

                enabled: false

                headerText: qsTr("Port")
                textField.text: port
            }

            AwgTextField {
                id: initPacketJunkSizeTextField

                enabled: false

                headerText: "S1 - Init packet junk size"
                textField.text: serverInitPacketJunkSize
            }

            AwgTextField {
                id: responsePacketJunkSizeTextField

                enabled: false

                headerText: "S2 - Response packet junk size"
                textField.text: serverResponsePacketJunkSize
            }

            AwgTextField {
                id: cookieReplyPacketJunkSizeTextField

                visible: isAwg2
                enabled: false

                headerText: "S3 - Cookie Reply packet junk size"
                textField.text: serverCookieReplyPacketJunkSize
            }

            AwgTextField {
                id: transportPacketJunkSizeTextField

                visible: isAwg2
                enabled: false

                headerText: "S4 - Transport packet junk size"
                textField.text: serverTransportPacketJunkSize
            }

            AwgTextField {
                id: initPacketMagicHeaderTextField

                enabled: false

                headerText: "H1 - Init packet magic header"
                textField.text: serverInitPacketMagicHeader
            }

            AwgTextField {
                id: responsePacketMagicHeaderTextField

                enabled: false

                headerText: "H2 - Response packet magic header"
                textField.text: serverResponsePacketMagicHeader
            }

            AwgTextField {
                id: underloadPacketMagicHeaderTextField

                enabled: false

                headerText: "H3 - Underload packet magic header"
                textField.text: serverUnderloadPacketMagicHeader
            }

            AwgTextField {
                id: transportPacketMagicHeaderTextField

                enabled: false

                headerText: "H4 - Transport packet magic header"
                textField.text: serverTransportPacketMagicHeader
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

        enabled: listView.currentItem !== null && listView.currentItem.isSaveButtonEnabled

        text: qsTr("Save")

        onActiveFocusChanged: {
            if(activeFocus) {
                listView.positionViewAtEnd()
            }
        }

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

            showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
        }
    }
}
