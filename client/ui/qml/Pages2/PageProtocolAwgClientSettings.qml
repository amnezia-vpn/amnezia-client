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

    defaultActiveFocusItem: listview.currentItem.mtuTextField.textField

    Item {
        id: focusItem
        onFocusChanged: {
            if (activeFocus) {
                fl.ensureVisible(focusItem)
            }
        }
        KeyNavigation.tab: backButton
    }

    ColumnLayout {
        id: backButtonLayout

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        anchors.topMargin: 20

        BackButtonType {
            id: backButton
            KeyNavigation.tab: listview.currentItem.mtuTextField.textField
        }
    }

    FlickableType {
        id: fl
        anchors.top: backButtonLayout.bottom
        anchors.bottom: parent.bottom
        contentHeight: content.implicitHeight + saveButton.implicitHeight + saveButton.anchors.bottomMargin + saveButton.anchors.topMargin

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

                        HeaderType {
                            Layout.fillWidth: true

                            headerText: qsTr("AmneziaWG settings")
                        }

                        TextFieldWithHeaderType {
                            id: mtuTextField
                            Layout.fillWidth: true
                            Layout.topMargin: 40

                            headerText: qsTr("MTU")
                            textFieldText: clientMtu
                            textField.validator: IntValidator { bottom: 576; top: 65535 }

                            textField.onEditingFinished: {
                                if (textFieldText !== clientMtu) {
                                    clientMtu = textFieldText
                                }
                            }
                            checkEmptyText: true
                            KeyNavigation.tab: junkPacketCountTextField.textField
                        }

                        TextFieldWithHeaderType {
                            id: junkPacketCountTextField
                            Layout.fillWidth: true
                            Layout.topMargin: 16

                            headerText: "Jc - Junk packet count"
                            textFieldText: clientJunkPacketCount
                            textField.validator: IntValidator { bottom: 0 }
                            parentFlickable: fl

                            textField.onEditingFinished: {
                                if (textFieldText !== clientJunkPacketCount) {
                                    clientJunkPacketCount = textFieldText
                                }
                            }

                            checkEmptyText: true

                            KeyNavigation.tab: junkPacketMinSizeTextField.textField
                        }

                        TextFieldWithHeaderType {
                            id: junkPacketMinSizeTextField
                            Layout.fillWidth: true
                            Layout.topMargin: 16

                            headerText: "Jmin - Junk packet minimum size"
                            textFieldText: clientJunkPacketMinSize
                            textField.validator: IntValidator { bottom: 0 }
                            parentFlickable: fl

                            textField.onEditingFinished: {
                                if (textFieldText !== clientJunkPacketMinSize) {
                                    clientJunkPacketMinSize = textFieldText
                                }
                            }

                            checkEmptyText: true

                            KeyNavigation.tab: junkPacketMaxSizeTextField.textField
                        }

                        TextFieldWithHeaderType {
                            id: junkPacketMaxSizeTextField
                            Layout.fillWidth: true
                            Layout.topMargin: 16

                            headerText: "Jmax - Junk packet maximum size"
                            textFieldText: clientJunkPacketMaxSize
                            textField.validator: IntValidator { bottom: 0 }
                            parentFlickable: fl

                            textField.onEditingFinished: {
                                if (textFieldText !== clientJunkPacketMaxSize) {
                                    clientJunkPacketMaxSize = textFieldText
                                }
                            }

                            checkEmptyText: true

                        }

                        Header2TextType {
                            Layout.fillWidth: true
                            Layout.topMargin: 16

                            text: qsTr("Server settings")
                        }

                        TextFieldWithHeaderType {
                            id: portTextField
                            Layout.fillWidth: true
                            Layout.topMargin: 8

                            enabled: false

                            headerText: qsTr("Port")
                            textFieldText: port
                        }

                        TextFieldWithHeaderType {
                            id: initPacketJunkSizeTextField
                            Layout.fillWidth: true
                            Layout.topMargin: 16

                            enabled: false

                            headerText: "S1 - Init packet junk size"
                            textFieldText: serverInitPacketJunkSize
                        }

                        TextFieldWithHeaderType {
                            id: responsePacketJunkSizeTextField
                            Layout.fillWidth: true
                            Layout.topMargin: 16

                            enabled: false

                            headerText: "S2 - Response packet junk size"
                            textFieldText: serverResponsePacketJunkSize
                        }

                        TextFieldWithHeaderType {
                            id: initPacketMagicHeaderTextField
                            Layout.fillWidth: true
                            Layout.topMargin: 16

                            enabled: false

                            headerText: "H1 - Init packet magic header"
                            textFieldText: serverInitPacketMagicHeader
                        }

                        TextFieldWithHeaderType {
                            id: responsePacketMagicHeaderTextField
                            Layout.fillWidth: true
                            Layout.topMargin: 16

                            enabled: false

                            headerText: "H2 - Response packet magic header"
                            textFieldText: serverResponsePacketMagicHeader
                        }

                        TextFieldWithHeaderType {
                            id: underloadPacketMagicHeaderTextField
                            Layout.fillWidth: true
                            Layout.topMargin: 16
                            parentFlickable: fl

                            enabled: false

                            headerText: "H3 - Underload packet magic header"
                            textFieldText: serverUnderloadPacketMagicHeader
                        }

                        TextFieldWithHeaderType {
                            id: transportPacketMagicHeaderTextField
                            Layout.fillWidth: true
                            Layout.topMargin: 16

                            enabled: false

                            headerText: "H4 - Transport packet magic header"
                            textFieldText: serverTransportPacketMagicHeader
                        }
                    }
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
