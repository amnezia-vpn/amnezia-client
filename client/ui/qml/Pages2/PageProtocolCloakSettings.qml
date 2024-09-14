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

                model: CloakConfigModel

                delegate: Item {
                    implicitWidth: listview.width
                    implicitHeight: col.implicitHeight

                    property alias trafficFromField: trafficFromField

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

                            headerText: qsTr("Cloak settings")
                        }

                        TextFieldWithHeaderType {
                            id: trafficFromField

                            Layout.fillWidth: true
                            Layout.topMargin: 32

                            headerText: qsTr("Disguised as traffic from")
                            textFieldText: site

                            textField.onEditingFinished: {
                                if (textFieldText !== site) {
                                    var tmpText = textFieldText
                                    tmpText = tmpText.toLocaleLowerCase()

                                    var indexHttps = tmpText.indexOf("https://")
                                    if (indexHttps === 0) {
                                        tmpText = textFieldText.substring(8)
                                    } else {
                                        site = textFieldText
                                    }
                                }
                            }
                        }

                        TextFieldWithHeaderType {
                            id: portTextField

                            Layout.fillWidth: true
                            Layout.topMargin: 16

                            headerText: qsTr("Port")
                            textFieldText: port
                            textField.maximumLength: 5
                            textField.validator: IntValidator { bottom: 1; top: 65535 }

                            textField.onEditingFinished: {
                                if (textFieldText !== port) {
                                    port = textFieldText
                                }
                            }
                        }

                        DropDownType {
                            id: cipherDropDown
                            Layout.fillWidth: true
                            Layout.topMargin: 16

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
                                    cipherDropDown.close()
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
                            id: saveRestartButton

                            Layout.fillWidth: true
                            Layout.topMargin: 24
                            Layout.bottomMargin: 24

                            text: qsTr("Save")
                            Keys.onTabPressed: lastItemTabClicked(focusItem)

                            clickedFunc: function() {
                                forceActiveFocus()

                                if (ConnectionController.isConnected && ServersModel.getDefaultServerData("defaultContainer") === ContainersModel.getProcessedContainerIndex()) {
                                    PageController.showNotificationMessage(qsTr("Unable change settings while there is an active connection"))
                                    return
                                }

                                PageController.goToPage(PageEnum.PageSetupWizardInstalling);
                                InstallController.updateContainer(CloakConfigModel.getConfig())
                            }
                        }
                    }
                }
            }
        }
    }
}
