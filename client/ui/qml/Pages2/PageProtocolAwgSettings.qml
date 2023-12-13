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

            enabled: ServersModel.isCurrentlyProcessedServerHasWriteAccess()

            ListView {
                id: listview

                width: parent.width
                height: listview.contentItem.height

                clip: true
                interactive: false

                model: AwgConfigModel

                delegate: Item {
                    implicitWidth: listview.width
                    implicitHeight: col.implicitHeight

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
                        }

                        TextFieldWithHeaderType {
                            id: junkPacketCountTextField
                            Layout.fillWidth: true
                            Layout.topMargin: 16

                            headerText: qsTr("Junk packet count")
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
                        }

                        TextFieldWithHeaderType {
                            id: junkPacketMinSizeTextField
                            Layout.fillWidth: true
                            Layout.topMargin: 16

                            headerText: qsTr("Junk packet minimum size")
                            textFieldText: junkPacketMinSize
                            textField.validator: IntValidator { bottom: 0 }

                            textField.onEditingFinished: {
                                if (textFieldText !== junkPacketMinSize) {
                                    junkPacketMinSize = textFieldText
                                }
                            }

                            checkEmptyText: true
                        }

                        TextFieldWithHeaderType {
                            id: junkPacketMaxSizeTextField
                            Layout.fillWidth: true
                            Layout.topMargin: 16

                            headerText: qsTr("Junk packet maximum size")
                            textFieldText: junkPacketMaxSize
                            textField.validator: IntValidator { bottom: 0 }

                            textField.onEditingFinished: {
                                if (textFieldText !== junkPacketMaxSize) {
                                    junkPacketMaxSize = textFieldText
                                }
                            }

                            checkEmptyText: true
                        }

                        TextFieldWithHeaderType {
                            id: initPacketJunkSizeTextField
                            Layout.fillWidth: true
                            Layout.topMargin: 16

                            headerText: qsTr("Init packet junk size")
                            textFieldText: initPacketJunkSize
                            textField.validator: IntValidator { bottom: 0 }

                            textField.onEditingFinished: {
                                if (textFieldText !== initPacketJunkSize) {
                                    initPacketJunkSize = textFieldText
                                }
                            }

                            checkEmptyText: true
                        }

                        TextFieldWithHeaderType {
                            id: responsePacketJunkSizeTextField
                            Layout.fillWidth: true
                            Layout.topMargin: 16

                            headerText: qsTr("Response packet junk size")
                            textFieldText: responsePacketJunkSize
                            textField.validator: IntValidator { bottom: 0 }

                            textField.onEditingFinished: {
                                if (textFieldText !== responsePacketJunkSize) {
                                    responsePacketJunkSize = textFieldText
                                }
                            }

                            checkEmptyText: true
                        }

                        TextFieldWithHeaderType {
                            id: initPacketMagicHeaderTextField
                            Layout.fillWidth: true
                            Layout.topMargin: 16

                            headerText: qsTr("Init packet magic header")
                            textFieldText: initPacketMagicHeader
                            textField.validator: IntValidator { bottom: 0 }

                            textField.onEditingFinished: {
                                if (textFieldText !== initPacketMagicHeader) {
                                    initPacketMagicHeader = textFieldText
                                }
                            }

                            checkEmptyText: true
                        }

                        TextFieldWithHeaderType {
                            id: responsePacketMagicHeaderTextField
                            Layout.fillWidth: true
                            Layout.topMargin: 16

                            headerText: qsTr("Response packet magic header")
                            textFieldText: responsePacketMagicHeader
                            textField.validator: IntValidator { bottom: 0 }

                            textField.onEditingFinished: {
                                if (textFieldText !== responsePacketMagicHeader) {
                                    responsePacketMagicHeader = textFieldText
                                }
                            }

                            checkEmptyText: true
                        }

                        TextFieldWithHeaderType {
                            id: transportPacketMagicHeaderTextField
                            Layout.fillWidth: true
                            Layout.topMargin: 16

                            headerText: qsTr("Transport packet magic header")
                            textFieldText: transportPacketMagicHeader
                            textField.validator: IntValidator { bottom: 0 }

                            textField.onEditingFinished: {
                                if (textFieldText !== transportPacketMagicHeader) {
                                    transportPacketMagicHeader = textFieldText
                                }
                            }

                            checkEmptyText: true
                        }

                        TextFieldWithHeaderType {
                            id: underloadPacketMagicHeaderTextField
                            Layout.fillWidth: true
                            Layout.topMargin: 16

                            headerText: qsTr("Underload packet magic header")
                            textFieldText: underloadPacketMagicHeader
                            textField.validator: IntValidator { bottom: 0 }

                            textField.onEditingFinished: {
                                if (textFieldText !== underloadPacketMagicHeader) {
                                    underloadPacketMagicHeader = textFieldText
                                }
                            }

                            checkEmptyText: true
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
                                questionDrawer.headerText = qsTr("Remove AmneziaWG from server?")
                                questionDrawer.descriptionText = qsTr("All users with whom you shared a connection will no longer be able to connect to it.")
                                questionDrawer.yesButtonText = qsTr("Continue")
                                questionDrawer.noButtonText = qsTr("Cancel")

                                questionDrawer.yesButtonFunction = function() {
                                    questionDrawer.visible = false
                                    PageController.goToPage(PageEnum.PageDeinstalling)
                                    InstallController.removeCurrentlyProcessedContainer()
                                }
                                questionDrawer.noButtonFunction = function() {
                                    questionDrawer.visible = false
                                }
                                questionDrawer.visible = true
                            }
                        }

                        BasicButtonType {
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

                            onClicked: {
                                forceActiveFocus()
                                PageController.goToPage(PageEnum.PageSetupWizardInstalling);
                                InstallController.updateContainer(AwgConfigModel.getConfig())
                            }
                        }
                    }
                }
            }
        }

        QuestionDrawer {
            id: questionDrawer
        }
    }
}
