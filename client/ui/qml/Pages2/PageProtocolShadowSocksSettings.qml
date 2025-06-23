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

            enabled: ServersModel.isProcessedServerHasWriteAccess()

            ListView {
                id: listview

                width: parent.width
                height: listview.contentItem.height

                clip: true
                interactive: false

                model: ShadowSocksConfigModel

                delegate: Item {
                    id: delegateItem

                    property bool isEnabled: ServersModel.isProcessedServerHasWriteAccess()

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

                        BaseHeaderType {
                            Layout.fillWidth: true
                            headerText: qsTr("Shadowsocks settings")
                        }

                        TextFieldWithHeaderType {
                            id: portTextField

                            Layout.fillWidth: true
                            Layout.topMargin: 40

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

                        DropDownType {
                            id: cipherDropDown
                            Layout.fillWidth: true
                            Layout.topMargin: 20

                            enabled: delegateItem.isEnabled

                            descriptionText: qsTr("Cipher")
                            headerText: qsTr("Cipher")

                            drawerParent: root

                            listView: ListViewWithRadioButtonType {

                                id: cipherListView

                                rootWidth: root.width

                                model: ListModel {
                                    ListElement { name : "chacha20-ietf-poly1305" }
                                    ListElement { name : "xchacha20-ietf-poly1305" }
                                    ListElement { name : "aes-256-gcm" }
                                    ListElement { name : "aes-192-gcm" }
                                    ListElement { name : "aes-128-gcm" }
                                }

                                clickedFunction: function() {
                                    cipherDropDown.text = selectedText
                                    cipher = cipherDropDown.text
                                    cipherDropDown.closeTriggered()
                                }

                                Component.onCompleted: {
                                    cipherDropDown.text = cipher

                                    for (var i = 0; i < cipherListView.model.count; i++) {
                                        if (cipherListView.model.get(i).name === cipherDropDown.text) {
                                            currentIndex = i
                                        }
                                    }
                                }
                            }
                        }

                        BasicButtonType {
                            id: saveButton

                            Layout.fillWidth: true
                            Layout.topMargin: 24
                            Layout.bottomMargin: 24

                            enabled: portTextField.errorText === ""

                            text: qsTr("Save")

                            clickedFunc: function() {
                                forceActiveFocus()

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
                                    InstallController.updateContainer(ShadowSocksConfigModel.getConfig())
                                }
                                var noButtonFunction = function() {
                                    if (!GC.isMobile()) {
                                        saveButton.forceActiveFocus()
                                    }
                                }
                                showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                            }

                            Keys.onEnterPressed: saveButton.clicked()
                            Keys.onReturnPressed: saveButton.clicked()
                        }
                    }
                }
            }
        }
    }
}
