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

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20 + PageController.safeAreaTopMargin

        onActiveFocusChanged: {
            if(backButton.enabled && backButton.activeFocus) {
                listView.positionViewAtBeginning()
            }
        }
    }

    SmartScroll {
        id: smartScroll
        listView: listView
    }

    ListViewType {
        id: listView

        anchors.top: backButton.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        model: AwgConfigModel

        delegate: ColumnLayout {
            id: delegateItem

            width: listView.width

            property alias vpnAddressSubnetTextField: vpnAddressSubnetTextField
            property bool isEnabled: ServersUiController.isProcessedServerHasWriteAccess()

            spacing: 0

            BaseHeaderType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("AmneziaWG settings")
            }

            TextFieldWithHeaderType {
                id: vpnAddressSubnetTextField

                Layout.fillWidth: true
                Layout.topMargin: 40
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                enabled: delegateItem.isEnabled

                headerText: qsTr("VPN address subnet")
                textField.text: subnetAddress

                textField.onEditingFinished: {
                    if (textField.text !== subnetAddress) {
                        subnetAddress = textField.text
                    }
                }

                textField.onActiveFocusChanged: {
                    if (textField.activeFocus) {
                        smartScroll.scrollToItem(vpnAddressSubnetTextField)
                    }
                }

                checkEmptyText: true
            }

            TextFieldWithHeaderType {
                id: portTextField
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                enabled: delegateItem.isEnabled

                headerText: qsTr("Port")
                textField.text: port
                textField.maximumLength: 5
                textField.validator: IntValidator { bottom: 1; top: 65535 }

                textField.onEditingFinished: {
                    if (textField.text !== port) {
                        port = textField.text
                    }
                }

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
                textField.text: serverJunkPacketCount

                scroller: smartScroll
                onEdited: (text) => { serverJunkPacketCount = text }
            }

            AwgTextField {
                id: junkPacketMinSizeTextField

                headerText: qsTr("Jmin - Junk packet minimum size")
                textField.text: serverJunkPacketMinSize

                scroller: smartScroll
                onEdited: (text) => { serverJunkPacketMinSize = text }
            }

            AwgTextField {
                id: junkPacketMaxSizeTextField

                headerText: qsTr("Jmax - Junk packet maximum size")
                textField.text: serverJunkPacketMaxSize

                scroller: smartScroll
                onEdited: (text) => { serverJunkPacketMaxSize = text }
            }

            AwgTextField {
                id: initPacketJunkSizeTextField

                headerText: qsTr("S1 - Init packet junk size")
                textField.text: serverInitPacketJunkSize

                scroller: smartScroll
                onEdited: (text) => { serverInitPacketJunkSize = text }
            }

            AwgTextField {
                id: responsePacketJunkSizeTextField

                headerText: qsTr("S2 - Response packet junk size")
                textField.text: serverResponsePacketJunkSize

                scroller: smartScroll
                onEdited: (text) => { serverResponsePacketJunkSize = text }
            }

            AwgTextField {
                id: cookieReplyPacketJunkSizeTextField

                visible: isAwg2

                headerText: qsTr("S3 - Cookie reply packet junk size")
                textField.text: serverCookieReplyPacketJunkSize

                scroller: smartScroll
                onEdited: (text) => { serverCookieReplyPacketJunkSize = text }
            }

            AwgTextField {
                id: transportPacketJunkSizeTextField

                visible: isAwg2

                headerText: qsTr("S4 - Transport packet junk size")
                textField.text: serverTransportPacketJunkSize

                scroller: smartScroll
                onEdited: (text) => { serverTransportPacketJunkSize = text }
            }

            AwgTextField {
                id: initPacketMagicHeaderTextField

                rangeValidation: true

                headerText: qsTr("H1 - Init packet magic header")
                textField.text: serverInitPacketMagicHeader

                scroller: smartScroll
                onEdited: (text) => { serverInitPacketMagicHeader = text }
            }

            AwgTextField {
                id: responsePacketMagicHeaderTextField

                rangeValidation: true

                headerText: qsTr("H2 - Response packet magic header")
                textField.text: serverResponsePacketMagicHeader

                scroller: smartScroll
                onEdited: (text) => { serverResponsePacketMagicHeader = text }
            }

            AwgTextField {
                id: underloadPacketMagicHeaderTextField

                rangeValidation: true

                headerText: qsTr("H3 - Underload packet magic header")
                textField.text: serverUnderloadPacketMagicHeader

                scroller: smartScroll
                onEdited: (text) => { serverUnderloadPacketMagicHeader = text }
            }

            AwgTextField {
                id: transportPacketMagicHeaderTextField

                rangeValidation: true

                headerText: qsTr("H4 - Transport packet magic header")
                textField.text: serverTransportPacketMagicHeader

                scroller: smartScroll
                onEdited: (text) => { serverTransportPacketMagicHeader = text }
            }

            AwgTextField {
                id: specialJunk1TextField

                headerText: qsTr("I1 - Special junk 1")
                textField.text: serverSpecialJunk1

                scroller: smartScroll
                onEdited: (text) => { serverSpecialJunk1 = text }
            }

            AwgTextField {
                id: specialJunk2TextField

                headerText: qsTr("I2 - Special junk 2")
                textField.text: serverSpecialJunk2

                scroller: smartScroll
                onEdited: (text) => { serverSpecialJunk2 = text }
            }

            AwgTextField {
                id: specialJunk3TextField

                headerText: qsTr("I3 - Special junk 3")
                textField.text: serverSpecialJunk3

                scroller: smartScroll
                onEdited: (text) => { serverSpecialJunk3 = text }
            }

            AwgTextField {
                id: specialJunk4TextField

                headerText: qsTr("I4 - Special junk 4")
                textField.text: serverSpecialJunk4

                scroller: smartScroll
                onEdited: (text) => { serverSpecialJunk4 = text }
            }

            AwgTextField {
                id: specialJunk5TextField

                headerText: qsTr("I5 - Special junk 5")
                textField.text: serverSpecialJunk5

                scroller: smartScroll
                onEdited: (text) => { serverSpecialJunk5 = text }
            }

            CheckBoxType {
                id: headerProtectionCheckBox

                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                visible: isAwg3

                text: qsTr("HeaderProtectionKey")

                checked: serverHeaderProtectionEnabled
                onCheckedChanged: {
                    if (checked !== serverHeaderProtectionEnabled) {
                        serverHeaderProtectionEnabled = checked
                    }
                }
            }

            AwgTextField {
                id: contentPaddingAdditionTextField

                visible: isAwg3
                rangeValidation: true

                headerText: qsTr("ContentPaddingAddition - Content padding addition")
                textField.text: serverContentPaddingAddition

                scroller: smartScroll
                onEdited: (text) => { serverContentPaddingAddition = text }
            }

            AwgTextField {
                id: rekeyAfterTimeTextField

                visible: isAwg3
                rangeValidation: true

                headerText: qsTr("RekeyAfterTime - Rekey after time")
                textField.text: serverRekeyAfterTime

                scroller: smartScroll
                onEdited: (text) => { serverRekeyAfterTime = text }
            }

            AwgTextField {
                id: rekeyTimeoutTextField

                visible: isAwg3
                rangeValidation: true

                headerText: qsTr("RekeyTimeout - Rekey timeout")
                textField.text: serverRekeyTimeout

                scroller: smartScroll
                onEdited: (text) => { serverRekeyTimeout = text }
            }

            AwgTextField {
                id: rejectAfterTimeTextField

                visible: isAwg3
                rangeValidation: true

                headerText: qsTr("RejectAfterTime - Reject after time")
                textField.text: serverRejectAfterTime

                scroller: smartScroll
                onEdited: (text) => { serverRejectAfterTime = text }
            }

            AwgTextField {
                id: keepaliveTimeoutTextField

                visible: isAwg3
                rangeValidation: true

                headerText: qsTr("KeepaliveTimeout - Keepalive timeout")
                textField.text: serverKeepaliveTimeout

                scroller: smartScroll
                onEdited: (text) => { serverKeepaliveTimeout = text }
            }

            AwgTextField {
                id: maxHandshakeAttemptsTextField

                visible: isAwg3
                rangeValidation: true

                headerText: qsTr("MaxHandshakeAttempts - Max handshake attempts")
                textField.text: serverMaxHandshakeAttempts

                scroller: smartScroll
                onEdited: (text) => { serverMaxHandshakeAttempts = text }
            }

            CheckBoxType {
                id: randomTrailersCheckBox

                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                visible: isAwg3

                text: qsTr("RandomTrailers")

                checked: serverRandomTrailers
                onCheckedChanged: {
                    if (checked !== serverRandomTrailers) {
                        serverRandomTrailers = checked
                    }
                }
            }

            CheckBoxType {
                id: disableCookiesCheckBox

                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                visible: isAwg3

                text: qsTr("DisableCookies")

                checked: serverDisableCookies
                onCheckedChanged: {
                    if (checked !== serverDisableCookies) {
                        serverDisableCookies = checked
                    }
                }
            }

            BasicButtonType {
                id: saveRestartButton

                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.bottomMargin: 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                enabled: underloadPacketMagicHeaderTextField.errorText === "" &&
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

                text: qsTr("Save")

                onActiveFocusChanged: {
                    if(activeFocus) {
                        listView.positionViewAtEnd()
                    }
                }

                clickedFunc: function() {
                    forceActiveFocus()
                    if (delegateItem.isEnabled) {
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

                    showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                }
            }
        }
    }
}
