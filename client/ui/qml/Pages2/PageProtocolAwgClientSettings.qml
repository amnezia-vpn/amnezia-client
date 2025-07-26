import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0

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

        onActiveFocusChanged: {
            if(backButton.enabled && backButton.activeFocus) {
                listView.positionViewAtBeginning()
            }
        }
    }

    ListViewType {
        id: listView

        anchors.top: backButtonLayout.bottom
        anchors.bottom: saveButton.top
        anchors.right: parent.right
        anchors.left: parent.left

        header: ColumnLayout {
            width: listView.width
            
            BaseHeaderType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("AmneziaWG settings")
            }
        }

        model: AwgConfigModel

        delegate: ColumnLayout {
            width: listView.width

            property bool isSaveButtonEnabled: mtuTextField.errorText === "" &&
                                               junkPacketMaxSizeTextField.errorText === "" &&
                                               junkPacketMinSizeTextField.errorText === "" &&
                                               junkPacketCountTextField.errorText === ""

            spacing: 0

            TextFieldWithHeaderType {
                id: mtuTextField

                Layout.fillWidth: true
                Layout.topMargin: 40
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
                checkEmptyText: true
            }

            AwgTextField {
                id: junkPacketCountTextField

                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: "Jc - Junk packet count"
                textField.text: clientJunkPacketCount

                textField.onEditingFinished: {
                    if (textField.text !== clientJunkPacketCount) {
                        clientJunkPacketCount = textField.text
                    }
                }
            }

            AwgTextField {
                id: junkPacketMinSizeTextField

                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: "Jmin - Junk packet minimum size"
                textField.text: clientJunkPacketMinSize

                textField.onEditingFinished: {
                    if (textField.text !== clientJunkPacketMinSize) {
                        clientJunkPacketMinSize = textField.text
                    }
                }
            }

            AwgTextField {
                id: junkPacketMaxSizeTextField

                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: "Jmax - Junk packet maximum size"
                textField.text: clientJunkPacketMaxSize

                textField.onEditingFinished: {
                    if (textField.text !== clientJunkPacketMaxSize) {
                        clientJunkPacketMaxSize = textField.text
                    }
                }
            }

            AwgTextField {
                id: specialJunk1TextField

                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("I1 - First special junk packet")
                textField.text: clientSpecialJunk1
                textField.validator: null
                checkEmptyText: false

                textField.onEditingFinished: {
                    if (textField.text !== clientSpecialJunk1) {
                        clientSpecialJunk1 = textField.text
                    }
                }
            }

            AwgTextField {
                id: specialJunk2TextField

                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("I2 - Second special junk packet")
                textField.text: clientSpecialJunk2
                textField.validator: null
                checkEmptyText: false

                textField.onEditingFinished: {
                    if (textField.text !== clientSpecialJunk2) {
                        clientSpecialJunk2 = textField.text
                    }
                }
            }

            AwgTextField {
                id: specialJunk3TextField

                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("I3 - Third special junk packet")
                textField.text: clientSpecialJunk3
                textField.validator: null
                checkEmptyText: false

                textField.onEditingFinished: {
                    if (textField.text !== clientSpecialJunk3) {
                        clientSpecialJunk3 = textField.text
                    }
                }
            }

            AwgTextField {
                id: specialJunk4TextField
                
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("I4 - Fourth special junk packet")
                textField.text: clientSpecialJunk4
                textField.validator: null
                checkEmptyText: false

                textField.onEditingFinished: {
                    if (textField.text !== clientSpecialJunk4) {
                        clientSpecialJunk4 = textField.text
                    }
                }
            }

            AwgTextField {
                id: specialJunk5TextField

                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("I5 - Fifth special junk packet")
                textField.text: clientSpecialJunk5
                textField.validator: null
                checkEmptyText: false

                textField.onEditingFinished: {
                    if (textField.text !== clientSpecialJunk5 ) {
                        clientSpecialJunk5 = textField.text
                    }
                }
            }

            AwgTextField {
                id: controlledJunk1TextField

                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("J1 - First controlled junk packet")
                textField.text: clientControlledJunk1
                textField.validator: null
                checkEmptyText: false

                textField.onEditingFinished: {
                    if (textField.text !== clientControlledJunk1) {
                        clientControlledJunk1 = textField.text
                    }
                }
            }

            AwgTextField {
                id: controlledJunk2TextField

                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("J2 - Second controlled junk packet")
                textField.text: clientControlledJunk2
                textField.validator: null
                checkEmptyText: false

                textField.onEditingFinished: {
                    if (textField.text !== clientControlledJunk2) {
                        clientControlledJunk2 = textField.text
                    }
                }
            }

            AwgTextField {
                id: controlledJunk3TextField

                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("J3 - Third controlled junk packet")
                textField.text: clientControlledJunk3
                textField.validator: null
                checkEmptyText: false

                textField.onEditingFinished: {
                    if (textField.text !== clientControlledJunk3) {
                        clientControlledJunk3 = textField.text
                    }
                }
            }

            AwgTextField {
                id: iTimeTextField

                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("Itime - Special handshake timeout")
                textField.text: clientSpecialHandshakeTimeout
                checkEmptyText: false

                textField.onEditingFinished: {
                    if (textField.text !== clientSpecialHandshakeTimeout) {
                        clientSpecialHandshakeTimeout = textField.text
                    }
                }
            }

            Header2TextType {
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Server settings")
            }

            AwgTextField {
                id: portTextField

                Layout.leftMargin: 16
                Layout.rightMargin: 16

                enabled: false

                headerText: qsTr("Port")
                textField.text: port
            }

            AwgTextField {
                id: initPacketJunkSizeTextField

                Layout.leftMargin: 16
                Layout.rightMargin: 16

                enabled: false

                headerText: "S1 - Init packet junk size"
                textField.text: serverInitPacketJunkSize
            }

            AwgTextField {
                id: responsePacketJunkSizeTextField

                Layout.leftMargin: 16
                Layout.rightMargin: 16

                enabled: false

                headerText: "S2 - Response packet junk size"
                textField.text: serverResponsePacketJunkSize
            }

            // AwgTextField {
            //     id: cookieReplyPacketJunkSizeTextField

            //     Layout.leftMargin: 16
            //     Layout.rightMargin: 16

            //     enabled: false

            //     headerText: "S3 - Cookie Reply packet junk size"
            //     textField.text: serverCookieReplyPacketJunkSize
            // }

            // AwgTextField {
            //     id: transportPacketJunkSizeTextField

            //     Layout.leftMargin: 16
            //     Layout.rightMargin: 16

            //     enabled: false

            //     headerText: "S4 - Transport packet junk size"
            //     textField.text: serverTransportPacketJunkSize
            // }

            AwgTextField {
                id: initPacketMagicHeaderTextField

                Layout.leftMargin: 16
                Layout.rightMargin: 16

                enabled: false

                headerText: "H1 - Init packet magic header"
                textField.text: serverInitPacketMagicHeader
            }

            AwgTextField {
                id: responsePacketMagicHeaderTextField

                Layout.leftMargin: 16
                Layout.rightMargin: 16

                enabled: false

                headerText: "H2 - Response packet magic header"
                textField.text: serverResponsePacketMagicHeader
            }

            AwgTextField {
                id: underloadPacketMagicHeaderTextField

                Layout.leftMargin: 16
                Layout.rightMargin: 16

                enabled: false

                headerText: "H3 - Underload packet magic header"
                textField.text: serverUnderloadPacketMagicHeader
            }

            AwgTextField {
                id: transportPacketMagicHeaderTextField

                Layout.leftMargin: 16
                Layout.rightMargin: 16

                enabled: false

                headerText: "H4 - Transport packet magic header"
                textField.text: serverTransportPacketMagicHeader
            }
        }
    }

    BasicButtonType {
        id: saveButton

        anchors.right: root.right
        anchors.left: root.left
        anchors.bottom: root.bottom

        anchors.topMargin: 24
        anchors.bottomMargin: 24
        anchors.rightMargin: 16
        anchors.leftMargin: 16

        enabled: listView.currentItem.isSaveButtonEnabled

        text: qsTr("Save")

        onActiveFocusChanged: {
            if(activeFocus) {
                listView.positionViewAtEnd()
            }
        }

        clickedFunc: function() {
            var headerText = qsTr("Save settings?")
            var descriptionText = qsTr("Only the settings for this device will be changed")
            var yesButtonText = qsTr("Continue")
            var noButtonText = qsTr("Cancel")

            var yesButtonFunction = function() {
                if (ConnectionController.isConnected && ServersModel.getDefaultServerData("defaultContainer") === ContainersModel.getProcessedContainerIndex()) {
                    PageController.showNotificationMessage(qsTr("Unable change settings while there is an active connection"))
                    return
                }

                PageController.goToPage(PageEnum.PageSetupWizardInstalling);
                InstallController.updateContainer(AwgConfigModel.getConfig())
            }

            var noButtonFunction = function() {}

            showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
        }
    }
}
