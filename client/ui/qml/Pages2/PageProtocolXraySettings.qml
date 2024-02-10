import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ContainerEnum 1.0

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

                model: XrayConfigModel

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
                            headerText: qsTr("XRay settings")
                        }

                        TextFieldWithHeaderType {
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
                            Layout.topMargin: 24
                            Layout.leftMargin: -8
                            implicitHeight: 32

                            visible: ContainersModel.getCurrentlyProcessedContainerIndex() === ContainerEnum.Xray

                            defaultColor: "transparent"
                            hoveredColor: Qt.rgba(1, 1, 1, 0.08)
                            pressedColor: Qt.rgba(1, 1, 1, 0.12)
                            textColor: "#EB5757"

                            text: qsTr("Remove XRay")

                            onClicked: {
                                questionDrawer.headerText = qsTr("Remove XRay from server?")
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

                            text: qsTr("Save and Restart Amnezia")

                            onClicked: {
                                forceActiveFocus()
                                PageController.goToPage(PageEnum.PageSetupWizardInstalling);
                                InstallController.updateContainer(XrayConfigModel.getConfig())
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
