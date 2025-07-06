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

    ListView {
        id: listview

        anchors.top: backButtonLayout.bottom
        anchors.bottom: saveButton.top

        width: parent.width

        clip: true

        property bool isFocusable: true

        Keys.onTabPressed: {
            FocusController.nextKeyTabItem()
        }

        Keys.onBacktabPressed: {
            FocusController.previousKeyTabItem()
        }

        Keys.onUpPressed: {
            FocusController.nextKeyUpItem()
        }

        Keys.onDownPressed: {
            FocusController.nextKeyDownItem()
        }

        Keys.onLeftPressed: {
            FocusController.nextKeyLeftItem()
        }

        Keys.onRightPressed: {
            FocusController.nextKeyRightItem()
        }

        model: AwgConfigModel

        delegate: Item {
            id: delegateItem
            implicitWidth: listview.width
            implicitHeight: col.implicitHeight

            property alias mtuTextField: mtuTextField
            property bool isSaveButtonEnabled: mtuTextField.errorText === "" &&
                                               junkPacketMaxSizeTextField.errorText === "" &&
                                               junkPacketMinSizeTextField.errorText === "" &&
                                               junkPacketCountTextField.errorText === ""

            ColumnLayout {
                id: col

                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right

                anchors.leftMargin: 16
                anchors.rightMargin: 16

                spacing: 0

                BaseHeaderType {
                    Layout.fillWidth: true

                    headerText: qsTr("AmneziaWG settings")
                }

                TextFieldWithHeaderType {
                    id: mtuTextField
                    Layout.fillWidth: true
                    Layout.topMargin: 40

                    headerText: qsTr("MTU")
                    textField.text: clientMtu
                    textField.validator: IntValidator { bottom: 576; top: 65535 }

                    textField.onEditingFinished: {
                        if (textField.text !== clientMtu) {
                            clientMtu = textField.text
                        }
                    }
                    checkEmptyText: true
                    KeyNavigation.tab: junkPacketCountTextField.textField
                }

                AwgTextField {
                    id: junkPacketCountTextField
                    headerText: "Jc - Junk packet count"
                    textField.text: clientJunkPacketCount

                    textField.onEditingFinished: {
                        if (textField.text !== clientJunkPacketCount) {
                            clientJunkPacketCount = textField.text
                        }
                    }

                    KeyNavigation.tab: junkPacketMinSizeTextField.textField
                }

                AwgTextField {
                    id: junkPacketMinSizeTextField
                    headerText: "Jmin - Junk packet minimum size"
                    textField.text: clientJunkPacketMinSize

                    textField.onEditingFinished: {
                        if (textField.text !== clientJunkPacketMinSize) {
                            clientJunkPacketMinSize = textField.text
                        }
                    }

                    KeyNavigation.tab: junkPacketMaxSizeTextField.textField
                }

                AwgTextField {
                    id: junkPacketMaxSizeTextField
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

                // AwgTextField {
                //     id: cookieReplyPacketJunkSizeTextField
                //     enabled: false

                //     headerText: "S3 - Cookie Reply packet junk size"
                //     textField.text: serverCookieReplyPacketJunkSize
                // }

                // AwgTextField {
                //     id: transportPacketJunkSizeTextField
                //     enabled: false

                //     headerText: "S4 - Transport packet junk size"
                //     textField.text: serverTransportPacketJunkSize
                // }

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

        enabled: listview.currentItem.isSaveButtonEnabled

        text: qsTr("Save")

        onActiveFocusChanged: {
            if(activeFocus) {
                listview.positionViewAtEnd()
            }
        }

        clickedFunc: function() {
            forceActiveFocus()
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
            var noButtonFunction = function() {
                if (!GC.isMobile()) {
                    saveButton.forceActiveFocus()
                }
            }
            showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
        }
    }
}
