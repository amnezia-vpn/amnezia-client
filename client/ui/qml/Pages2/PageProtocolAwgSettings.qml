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

    defaultActiveFocusItem: listview.currentItem.portTextField.textField

    ColumnLayout {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        anchors.topMargin: 20

        BackButtonType {
        }
    }

    FlickableType {
        id: fl
        anchors.top: backButton.bottom
        anchors.bottom: parent.bottom
        contentHeight: content.implicitHeight

        Column {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            enabled: ServersModel.isProcessedServerHasWriteAccess()

            ListView {


                id: listview

                width: parent.width
                height: listview.contentItem.height

                clip: true
                interactive: false

                model: AwgConfigModel

                delegate: Item {
                    id: _delegate

                    implicitWidth: listview.width
                    implicitHeight: col.implicitHeight

                    property alias portTextField:portTextField

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

                            headerText: qsTr("Port")
                            textFieldText: port
                            textField.maximumLength: 5
                            textField.validator: IntValidator { bottom: 1; top: 65535 }

                            textField.onEditingFinished: {
                                if (textFieldText !== port) {
                                    port = textFieldText
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
                            textFieldText: junkPacketCount
                            textField.validator: IntValidator { bottom: 0 }

                            textField.onEditingFinished: {
                                console.log("1")
                                if (textFieldText === "") {
                                    textFieldText = "0"
                                }

                                if (textFieldText !== junkPacketCount) {
                                    junkPacketCount = textFieldText
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
                            textFieldText: junkPacketMinSize
                            textField.validator: IntValidator { bottom: 0 }

                            textField.onEditingFinished: {
                                if (textFieldText !== junkPacketMinSize) {
                                    junkPacketMinSize = textFieldText
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
                            textFieldText: junkPacketMaxSize
                            textField.validator: IntValidator { bottom: 0 }

                            textField.onEditingFinished: {
                                if (textFieldText !== junkPacketMaxSize) {
                                    junkPacketMaxSize = textFieldText
                                }
                            }

                            checkEmptyText: true

                            KeyNavigation.tab: initPacketJunkSizeTextField.textField
                        }

                        TextFieldWithHeaderType {
                            id: initPacketJunkSizeTextField
                            Layout.fillWidth: true
                            Layout.topMargin: 16

                            headerText: "S1 - Init packet junk size"
                            textFieldText: initPacketJunkSize
                            textField.validator: IntValidator { bottom: 0 }

                            textField.onEditingFinished: {
                                if (textFieldText !== initPacketJunkSize) {
                                    initPacketJunkSize = textFieldText
                                }
                            }

                            checkEmptyText: true

                            KeyNavigation.tab: responsePacketJunkSizeTextField.textField
                        }

                        TextFieldWithHeaderType {
                            id: responsePacketJunkSizeTextField
                            Layout.fillWidth: true
                            Layout.topMargin: 16

                            headerText: "S2 - Response packet junk size"
                            textFieldText: responsePacketJunkSize
                            textField.validator: IntValidator { bottom: 0 }

                            textField.onEditingFinished: {
                                if (textFieldText !== responsePacketJunkSize) {
                                    responsePacketJunkSize = textFieldText
                                }
                            }

                            checkEmptyText: true

                            KeyNavigation.tab: initPacketMagicHeaderTextField.textField
                        }

                        TextFieldWithHeaderType {
                            id: initPacketMagicHeaderTextField
                            Layout.fillWidth: true
                            Layout.topMargin: 16

                            headerText: "H1 - Init packet magic header"
                            textFieldText: initPacketMagicHeader
                            textField.validator: IntValidator { bottom: 0 }

                            textField.onEditingFinished: {
                                if (textFieldText !== initPacketMagicHeader) {
                                    initPacketMagicHeader = textFieldText
                                }
                            }

                            checkEmptyText: true

                            KeyNavigation.tab: responsePacketMagicHeaderTextField.textField
                        }

                        TextFieldWithHeaderType {
                            id: responsePacketMagicHeaderTextField
                            Layout.fillWidth: true
                            Layout.topMargin: 16

                            headerText: "H2 - Response packet magic header"
                            textFieldText: responsePacketMagicHeader
                            textField.validator: IntValidator { bottom: 0 }

                            textField.onEditingFinished: {
                                if (textFieldText !== responsePacketMagicHeader) {
                                    responsePacketMagicHeader = textFieldText
                                }
                            }

                            checkEmptyText: true

                            KeyNavigation.tab: transportPacketMagicHeaderTextField.textField
                        }

                        TextFieldWithHeaderType {
                            id: transportPacketMagicHeaderTextField
                            Layout.fillWidth: true
                            Layout.topMargin: 16

                            headerText: "H4 - Transport packet magic header"
                            textFieldText: transportPacketMagicHeader
                            textField.validator: IntValidator { bottom: 0 }

                            textField.onEditingFinished: {
                                if (textFieldText !== transportPacketMagicHeader) {
                                    transportPacketMagicHeader = textFieldText
                                }
                            }

                            checkEmptyText: true

                            KeyNavigation.tab: underloadPacketMagicHeaderTextField.textField
                        }

                        TextFieldWithHeaderType {
                            id: underloadPacketMagicHeaderTextField
                            Layout.fillWidth: true
                            Layout.topMargin: 16

                            headerText: "H3 - Underload packet magic header"
                            textFieldText: underloadPacketMagicHeader
                            textField.validator: IntValidator { bottom: 0 }

                            textField.onEditingFinished: {
                                if (textFieldText !== underloadPacketMagicHeader) {
                                    underloadPacketMagicHeader = textFieldText
                                }
                            }

                            checkEmptyText: true

                            KeyNavigation.tab: saveRestartButton
                        }

                        BasicButtonType {
                            Layout.topMargin: 24
                            Layout.leftMargin: -8
                            implicitHeight: 32

                            defaultColor: "transparent"
                            hoveredColor: Qt.rgba(1, 1, 1, 0.08)
                            pressedColor: Qt.rgba(1, 1, 1, 0.12)
                            textColor: "#EB5757"

                            text: qsTr("Remove AmneziaWG")

                            onClicked: {
                                var headerText = qsTr("Remove AmneziaWG from server?")
                                var descriptionText = qsTr("All users with whom you shared a connection will no longer be able to connect to it.")
                                var yesButtonText = qsTr("Continue")
                                var noButtonText = qsTr("Cancel")

                                var yesButtonFunction = function() {
                                    PageController.goToPage(PageEnum.PageDeinstalling)
                                    InstallController.removeCurrentlyProcessedContainer()
                                }
                                var noButtonFunction = function() {
                                }
                                showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                            }
                        }

                        BasicButtonType {
                            id: saveRestartButton

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

                            text: qsTr("Save and Restart Amnezia")

                            clickedFunc: function() {
                                forceActiveFocus()
                                PageController.goToPage(PageEnum.PageSetupWizardInstalling);
                                InstallController.updateContainer(AwgConfigModel.getConfig())
                            }
                        }
                    }
                }

            }
        }
    }
}
