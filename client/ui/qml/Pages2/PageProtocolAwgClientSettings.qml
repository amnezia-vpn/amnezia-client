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

                TextFieldWithHeaderType {
                    id: junkPacketCountTextField
                    Layout.fillWidth: true
                    Layout.topMargin: 16

                    headerText: "Jc - Junk packet count"
                    textField.text: clientJunkPacketCount
                    textField.validator: IntValidator { bottom: 0 }

                    textField.onEditingFinished: {
                        if (textField.text !== clientJunkPacketCount) {
                            clientJunkPacketCount = textField.text
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
                    textField.text: clientJunkPacketMinSize
                    textField.validator: IntValidator { bottom: 0 }

                    textField.onEditingFinished: {
                        if (textField.text !== clientJunkPacketMinSize) {
                            clientJunkPacketMinSize = textField.text
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
                    textField.text: clientJunkPacketMaxSize
                    textField.validator: IntValidator { bottom: 0 }

                    textField.onEditingFinished: {
                        if (textField.text !== clientJunkPacketMaxSize) {
                            clientJunkPacketMaxSize = textField.text
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
                    textField.text: port
                }

                TextFieldWithHeaderType {
                    id: initPacketJunkSizeTextField
                    Layout.fillWidth: true
                    Layout.topMargin: 16

                    enabled: false

                    headerText: "S1 - Init packet junk size"
                    textField.text: serverInitPacketJunkSize
                }

                TextFieldWithHeaderType {
                    id: responsePacketJunkSizeTextField
                    Layout.fillWidth: true
                    Layout.topMargin: 16

                    enabled: false

                    headerText: "S2 - Response packet junk size"
                    textField.text: serverResponsePacketJunkSize
                }

                TextFieldWithHeaderType {
                    id: initPacketMagicHeaderTextField
                    Layout.fillWidth: true
                    Layout.topMargin: 16

                    enabled: false

                    headerText: "H1 - Init packet magic header"
                    textField.text: serverInitPacketMagicHeader
                }

                TextFieldWithHeaderType {
                    id: responsePacketMagicHeaderTextField
                    Layout.fillWidth: true
                    Layout.topMargin: 16

                    enabled: false

                    headerText: "H2 - Response packet magic header"
                    textField.text: serverResponsePacketMagicHeader
                }

                TextFieldWithHeaderType {
                    id: underloadPacketMagicHeaderTextField
                    Layout.fillWidth: true
                    Layout.topMargin: 16

                    enabled: false

                    headerText: "H3 - Underload packet magic header"
                    textField.text: serverUnderloadPacketMagicHeader
                }

                TextFieldWithHeaderType {
                    id: transportPacketMagicHeaderTextField
                    Layout.fillWidth: true
                    Layout.topMargin: 16

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
