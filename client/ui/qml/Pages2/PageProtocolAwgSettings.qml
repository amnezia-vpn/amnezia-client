import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QtCore

import SortFilterProxyModel 0.2

import PageEnum 1.0
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
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        model: AwgConfigModel

        delegate: ColumnLayout {
            id: delegateItem

            width: listView.width

            property alias vpnAddressSubnetTextField: vpnAddressSubnetTextField
            property bool isEnabled: ServersModel.isProcessedServerHasWriteAccess()

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

                checkEmptyText: true
            }

            AwgTextField {
                id: junkPacketCountTextField

                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("Jc - Junk packet count")
                textField.text: serverJunkPacketCount

                textField.onEditingFinished: {
                    if (textField.text !== serverJunkPacketCount) {
                        serverJunkPacketCount = textField.text
                    }
                }
            }

            AwgTextField {
                id: junkPacketMinSizeTextField

                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("Jmin - Junk packet minimum size")
                textField.text: serverJunkPacketMinSize

                textField.onEditingFinished: {
                    if (textField.text !== serverJunkPacketMinSize) {
                        serverJunkPacketMinSize = textField.text
                    }
                }
            }

            AwgTextField {
                id: junkPacketMaxSizeTextField

                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("Jmax - Junk packet maximum size")
                textField.text: serverJunkPacketMaxSize

                textField.onEditingFinished: {
                    if (textField.text !== serverJunkPacketMaxSize) {
                        serverJunkPacketMaxSize = textField.text
                    }
                }
            }

            AwgTextField {
                id: initPacketJunkSizeTextField

                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("S1 - Init packet junk size")
                textField.text: serverInitPacketJunkSize

                textField.onEditingFinished: {
                    if (textField.text !== serverInitPacketJunkSize) {
                        serverInitPacketJunkSize = textField.text
                    }
                }
            }

            AwgTextField {
                id: responsePacketJunkSizeTextField

                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("S2 - Response packet junk size")
                textField.text: serverResponsePacketJunkSize

                textField.onEditingFinished: {
                    if (textField.text !== serverResponsePacketJunkSize) {
                        serverResponsePacketJunkSize = textField.text
                    }
                }
            }

            // AwgTextField {
            //     id: cookieReplyPacketJunkSizeTextField

            //     Layout.leftMargin: 16
            //     Layout.rightMargin: 16

            //     headerText: qsTr("S3 - Cookie reply packet junk size")
            //     textField.text: serverCookieReplyPacketJunkSize

            //     textField.onEditingFinished: {
            //         if (textField.text !== serverCookieReplyPacketJunkSize) {
            //             serverCookieReplyPacketJunkSize = textField.text
            //         }
            //     }
            // }

            // AwgTextField {
            //     id: transportPacketJunkSizeTextField

            //     Layout.leftMargin: 16
            //     Layout.rightMargin: 16

            //     headerText: qsTr("S4 - Transport packet junk size")
            //     textField.text: serverTransportPacketJunkSize

            //     textField.onEditingFinished: {
            //         if (textField.text !== serverTransportPacketJunkSize) {
            //             serverTransportPacketJunkSize = textField.text
            //         }
            //     }
            // }

            AwgTextField {
                id: initPacketMagicHeaderTextField

                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("H1 - Init packet magic header")
                textField.text: serverInitPacketMagicHeader

                textField.onEditingFinished: {
                    if (textField.text !== serverInitPacketMagicHeader) {
                        serverInitPacketMagicHeader = textField.text
                    }
                }
            }

            AwgTextField {
                id: responsePacketMagicHeaderTextField

                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("H2 - Response packet magic header")
                textField.text: serverResponsePacketMagicHeader

                textField.onEditingFinished: {
                    if (textField.text !== serverResponsePacketMagicHeader) {
                        serverResponsePacketMagicHeader = textField.text
                    }
                }
            }

            AwgTextField {
                id: underloadPacketMagicHeaderTextField

                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("H3 - Underload packet magic header")
                textField.text: serverUnderloadPacketMagicHeader

                textField.onEditingFinished: {
                    if (textField.text !== serverUnderloadPacketMagicHeader) {
                        serverUnderloadPacketMagicHeader = textField.text
                    }
                }
            }

            AwgTextField {
                id: transportPacketMagicHeaderTextField

                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("H4 - Transport packet magic header")
                textField.text: serverTransportPacketMagicHeader

                textField.onEditingFinished: {
                    if (textField.text !== serverTransportPacketMagicHeader) {
                        serverTransportPacketMagicHeader = textField.text
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
                         responsePacketJunkSizeTextField.errorText === "" &&
                         // cookieReplyHeaderJunkTextField.errorText === "" &&
                         // transportHeaderJunkTextField.errorText === "" &&
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
                    if (delegateItem.isEnabled) {
                        if (AwgConfigModel.isHeadersEqual(underloadPacketMagicHeaderTextField.textField.text,
                                                          transportPacketMagicHeaderTextField.textField.text,
                                                          responsePacketMagicHeaderTextField.textField.text,
                                                          initPacketMagicHeaderTextField.textField.text)) {
                            PageController.showErrorMessage(qsTr("The values of the H1-H4 fields must be unique"))
                            return
                        }

                        if (AwgConfigModel.isPacketSizeEqual(parseInt(initPacketJunkSizeTextField.textField.text),
                                                             parseInt(responsePacketJunkSizeTextField.textField.text))) {
                            PageController.showErrorMessage(qsTr("The value of the field S1 + message initiation size (148) must not equal S2 + message response size (92)"))
                            return
                        }
                        // if (AwgConfigModel.isPacketSizeEqual(parseInt(initPacketJunkSizeTextField.textField.text),
                        //                                     parseInt(responsePacketJunkSizeTextField.textField.text),
                        //                                     parseInt(cookieReplyPacketJunkSizeTextField.textField.text),
                        //                                     parseInt(transportPacketJunkSizeTextField.textField.text))) {
                        //     PageController.showErrorMessage(qsTr("The value of the field S1 + message initiation size (148) must not equal S2 + message response size (92) + S3 + cookie reply size (64) + S4 + transport packet size (32)"))
                        //     return
                        // }
                    }

                    var headerText = qsTr("Save settings?")
                    var descriptionText = qsTr("All users with whom you shared a connection with will no longer be able to connect to it.")
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
    }
}
