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

    defaultActiveFocusItem: listview

    Item {
        id: focusItem
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
            KeyNavigation.tab: listview
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

                model: WireGuardConfigModel

                activeFocusOnTab: true
                onActiveFocusChanged: {
                    if (activeFocus) {
                        listview.itemAtIndex(0)?.focusItemId.forceActiveFocus()
                    }
                }

                delegate: Item {
                    property alias focusItemId: portTextField.textField

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
                            headerText: qsTr("WG settings")
                        }

                        TextFieldWithHeaderType {
                            id: portTextField
                            Layout.fillWidth: true
                            Layout.topMargin: 40

                            headerText: qsTr("Port")
                            textFieldText: port
                            textField.maximumLength: 5
                            textField.validator: IntValidator { bottom: 1; top: 65535 }

                            KeyNavigation.tab: mtuTextField.textField

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

                            KeyNavigation.tab: saveButton

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

                        BasicButtonType {
                            id: saveButton
                            Layout.fillWidth: true
                            Layout.topMargin: 24
                            Layout.bottomMargin: 24

                            enabled: mtuTextField.errorText === "" &&
                                     portTextField.errorText === ""

                            text: qsTr("Save")

                            Keys.onTabPressed: lastItemTabClicked(focusItem)

                            onClicked: {
                                forceActiveFocus()

                                if (ConnectionController.isConnected && ServersModel.getDefaultServerData("defaultContainer") === ContainersModel.getProcessedContainerIndex()) {
                                    PageController.showNotificationMessage(qsTr("Unable change settings while there is an active connection"))
                                    return
                                }

                                PageController.goToPage(PageEnum.PageSetupWizardInstalling);
                                InstallController.updateContainer(WireGuardConfigModel.getConfig())
                                focusItem.forceActiveFocus()
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
