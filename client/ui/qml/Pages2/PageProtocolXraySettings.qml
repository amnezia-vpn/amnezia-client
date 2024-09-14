import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ContainerEnum 1.0
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

                model: XrayConfigModel

                // activeFocusOnTab: true
                // onActiveFocusChanged: {
                //     if (activeFocus) {
                //         listview.itemAtIndex(0)?.focusItemId.forceActiveFocus()
                //     }
                // }

                delegate: Item {
                    property alias focusItemId: textFieldWithHeaderType.textField

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
                            headerText: qsTr("XRay settings")
                        }

                        TextFieldWithHeaderType {
                            id: textFieldWithHeaderType
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

                        BasicButtonType {
                            id: basicButton
                            Layout.fillWidth: true
                            Layout.topMargin: 24
                            Layout.bottomMargin: 24

                            text: qsTr("Save")

                            Keys.onTabPressed: lastItemTabClicked(focusItem)

                            onClicked: {
                                forceActiveFocus()

                                if (ConnectionController.isConnected && ServersModel.getDefaultServerData("defaultContainer") === ContainersModel.getProcessedContainerIndex()) {
                                    PageController.showNotificationMessage(qsTr("Unable change settings while there is an active connection"))
                                    return
                                }

                                PageController.goToPage(PageEnum.PageSetupWizardInstalling);
                                InstallController.updateContainer(XrayConfigModel.getConfig())
                                focusItem.forceActiveFocus()
                            }

                            Keys.onEnterPressed: basicButton.clicked()
                            Keys.onReturnPressed: basicButton.clicked()
                        }
                    }
                }
            }
        }
    }

}
