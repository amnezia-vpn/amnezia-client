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

        anchors.top: backButton.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        property int selectedIndex: 0

        enabled: ServersModel.isProcessedServerHasWriteAccess()

        header: ColumnLayout {
            width: listView.width

            BaseHeaderType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("Cloak settings")
            }
        }

        model: CloakConfigModel

        delegate: ColumnLayout {
            width: listView.width

            property alias trafficFromField: trafficFromField

            spacing: 0

            TextFieldWithHeaderType {
                id: trafficFromField

                Layout.fillWidth: true
                Layout.topMargin: 32
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("Disguised as traffic from")
                textField.text: site

                textField.onEditingFinished: {
                    if (textField.text !== site) {
                        var tmpText = textField.text
                        tmpText = tmpText.toLocaleLowerCase()

                        var indexHttps = tmpText.indexOf("https://")
                        if (indexHttps === 0) {
                            tmpText = textField.text.substring(8)
                        } else {
                            site = textField.text
                        }
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
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

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
                                selectedIndex = i
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
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                enabled: trafficFromField.errorText === "" &&
                         portTextField.errorText === ""

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

                        PageController.goToPage(PageEnum.PageSetupWizardInstalling)
                        InstallController.updateContainer(CloakConfigModel.getConfig())
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
