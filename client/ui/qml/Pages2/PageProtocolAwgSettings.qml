import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

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

    FlickableType {
        id: fl
        anchors.top: backButtonLayout.bottom
        anchors.bottom: parent.bottom
        contentHeight: content.implicitHeight

        Column {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            ListView {
                id: listview

                width: parent.width
                height: listview.contentItem.height

                clip: true
                interactive: false

                model: AwgConfigModel

                delegate: Item {
                    id: delegateItem
                    implicitWidth: listview.width
                    implicitHeight: col.implicitHeight

                    property alias portTextField: portTextField
                    property bool isEnabled: ServersModel.isProcessedServerHasWriteAccess()

                    ColumnLayout {
                        id: col

                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right

                        anchors.leftMargin: 16
                        anchors.rightMargin: 16

                        spacing: 0

                        HeaderType {
                            Layout.fillWidth: true

                            headerText: qsTr("AmneziaWG settings")
                        }

                        TextFieldWithHeaderType {
                            id: portTextField
                            Layout.fillWidth: true
                            Layout.topMargin: 40

                            enabled: delegateItem.isEnabled

                            headerText: qsTr("Port")
                            textFieldText: port
                            textField.maximumLength: 5
                            textField.validator: IntValidator { bottom: 1; top: 65535 }
                            parentFlickable: fl

                            textField.onEditingFinished: {
                                if (textFieldText !== port) {
                                    port = textFieldText
                                }
                            }

                            checkEmptyText: true
                        }

                        TextFieldWithHeaderType {
                            id: mtuTextField
                            Layout.fillWidth: true
                            Layout.topMargin: 16

                            headerText: qsTr("MTU")
                            textFieldText: mtu
                            textField.validator: IntValidator { bottom: 576; top: 65535 }

                            textField.onEditingFinished: {
                                if (textFieldText === "") {
                                    textFieldText = "0"
                                }
                                if (textFieldText !== mtu) {
                                    mtu = textFieldText
                                }
                            }
                            checkEmptyText: true
                        }

                        TextFieldWithHeaderType {
                            id: junkPacketCountTextField
                            Layout.fillWidth: true
                            Layout.topMargin: 16

                            headerText: qsTr("Jc - Junk packet count")
                            textFieldText: serverJunkPacketCount
                            textField.validator: IntValidator { bottom: 0 }
                            parentFlickable: fl

                            textField.onEditingFinished: {
                                if (textFieldText === "") {
                                    textFieldText = "0"
                                }

                                if (textFieldText !== serverJunkPacketCount) {
                                    serverJunkPacketCount = textFieldText
                                }
                            }

                            checkEmptyText: true
                        }

                        TextFieldWithHeaderType {
                            id: junkPacketMinSizeTextField
                            Layout.fillWidth: true
                            Layout.topMargin: 16

                            headerText: qsTr("Jmin - Junk packet minimum size")
                            textFieldText: serverJunkPacketMinSize
                            textField.validator: IntValidator { bottom: 0 }
                            parentFlickable: fl

                            textField.onEditingFinished: {
                                if (textFieldText !== serverJunkPacketMinSize) {
                                    serverJunkPacketMinSize = textFieldText
                                }
                            }

                            checkEmptyText: true
                        }

                        TextFieldWithHeaderType {
                            id: junkPacketMaxSizeTextField
                            Layout.fillWidth: true
                            Layout.topMargin: 16

                            headerText: qsTr("Jmax - Junk packet maximum size")
                            textFieldText: serverJunkPacketMaxSize
                            textField.validator: IntValidator { bottom: 0 }
                            parentFlickable: fl

                            textField.onEditingFinished: {
                                if (textFieldText !== serverJunkPacketMaxSize) {
                                    serverJunkPacketMaxSize = textFieldText
                                }
                            }

                            checkEmptyText: true
                        }

                        TextFieldWithHeaderType {
                            id: initPacketJunkSizeTextField
                            Layout.fillWidth: true
                            Layout.topMargin: 16

                            headerText: qsTr("S1 - Init packet junk size")
                            textFieldText: serverInitPacketJunkSize
                            textField.validator: IntValidator { bottom: 0 }
                            parentFlickable: fl

                            textField.onEditingFinished: {
                                if (textFieldText !== serverInitPacketJunkSize) {
                                    serverInitPacketJunkSize = textFieldText
                                }
                            }

                            checkEmptyText: true
                        }

                        TextFieldWithHeaderType {
                            id: responsePacketJunkSizeTextField
                            Layout.fillWidth: true
                            Layout.topMargin: 16

                            headerText: qsTr("S2 - Response packet junk size")
                            textFieldText: serverResponsePacketJunkSize
                            textField.validator: IntValidator { bottom: 0 }
                            parentFlickable: fl

                            textField.onEditingFinished: {
                                if (textFieldText !== serverResponsePacketJunkSize) {
                                    serverResponsePacketJunkSize = textFieldText
                                }
                            }

                            checkEmptyText: true
                        }

                        TextFieldWithHeaderType {
                            id: initPacketMagicHeaderTextField
                            Layout.fillWidth: true
                            Layout.topMargin: 16

                            headerText: qsTr("H1 - Init packet magic header")
                            textFieldText: serverInitPacketMagicHeader
                            textField.validator: IntValidator { bottom: 0 }
                            parentFlickable: fl

                            textField.onEditingFinished: {
                                if (textFieldText !== serverInitPacketMagicHeader) {
                                    serverInitPacketMagicHeader = textFieldText
                                }
                            }

                            checkEmptyText: true
                        }

                        TextFieldWithHeaderType {
                            id: responsePacketMagicHeaderTextField
                            Layout.fillWidth: true
                            Layout.topMargin: 16

                            headerText: qsTr("H2 - Response packet magic header")
                            textFieldText: serverResponsePacketMagicHeader
                            textField.validator: IntValidator { bottom: 0 }
                            parentFlickable: fl

                            textField.onEditingFinished: {
                                if (textFieldText !== serverResponsePacketMagicHeader) {
                                    serverResponsePacketMagicHeader = textFieldText
                                }
                            }

                            checkEmptyText: true
                        }

                        TextFieldWithHeaderType {
                            id: transportPacketMagicHeaderTextField
                            Layout.fillWidth: true
                            Layout.topMargin: 16

                            headerText: qsTr("H4 - Transport packet magic header")
                            textFieldText: serverTransportPacketMagicHeader
                            textField.validator: IntValidator { bottom: 0 }
                            parentFlickable: fl

                            textField.onEditingFinished: {
                                if (textFieldText !== serverTransportPacketMagicHeader) {
                                    serverTransportPacketMagicHeader = textFieldText
                                }
                            }

                            checkEmptyText: true
                        }

                        TextFieldWithHeaderType {
                            id: underloadPacketMagicHeaderTextField
                            Layout.fillWidth: true
                            Layout.topMargin: 16
                            parentFlickable: fl

                            headerText: qsTr("H3 - Underload packet magic header")
                            textFieldText: serverUnderloadPacketMagicHeader
                            textField.validator: IntValidator { bottom: 0 }

                            textField.onEditingFinished: {
                                if (textFieldText !== serverUnderloadPacketMagicHeader) {
                                    serverUnderloadPacketMagicHeader = textFieldText
                                }
                            }

                            checkEmptyText: true
                        }

                        BasicButtonType {
                            id: saveRestartButton
                            parentFlickable: fl

                            Layout.fillWidth: true
                            Layout.topMargin: 24
                            Layout.bottomMargin: 24

                            enabled: underloadPacketMagicHeaderTextField.errorText === "" &&
                                     transportPacketMagicHeaderTextField.errorText === "" &&
                                     responsePacketMagicHeaderTextField.errorText === "" &&
                                     initPacketMagicHeaderTextField.errorText === "" &&
                                     responsePacketJunkSizeTextField.errorText === "" &&
                                     initPacketJunkSizeTextField.errorText === "" &&
                                     junkPacketMaxSizeTextField.errorText === "" &&
                                     junkPacketMinSizeTextField.errorText === "" &&
                                     junkPacketCountTextField.errorText === "" &&
                                     portTextField.errorText === ""

                            text: qsTr("Save")

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

                                    if (AwgConfigModel.isPacketSizeEqual(parseInt(initPacketJunkSizeTextField.textField.text),
                                                                         parseInt(responsePacketJunkSizeTextField.textField.text))) {
                                        PageController.showErrorMessage(qsTr("The value of the field S1 + message initiation size (148) must not equal S2 + message response size (92)"))
                                        return
                                    }
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
                                var noButtonFunction = function() {
                                    if (!GC.isMobile()) {
                                        saveRestartButton.forceActiveFocus()
                                    }
                                }
                                showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                            }
                        }
                    }
                }
            }
        }
    }
}
