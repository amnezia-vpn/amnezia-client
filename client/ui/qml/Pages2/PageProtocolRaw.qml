import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ProtocolEnum 1.0
import ContainerEnum 1.0
import ContainerProps 1.0
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

        onFocusChanged: {
            if (this.activeFocus) {
                listView.positionViewAtBeginning()
            }
        }
    }

    ListViewType {
        id: listView

        anchors.top: backButton.bottom
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.left: parent.left

        header: ColumnLayout {
            width: listView.width

            BaseHeaderType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 16

                headerText: ContainersModel.getProcessedContainerName() + qsTr(" settings")
            }
        }

        model: ProtocolsModel

        delegate: ColumnLayout {
            width: listView.width

            LabelWithButtonType {
                id: button

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Show connection options")

                clickedFunction: function() {
                    configContentDrawer.openTriggered()
                }

                MouseArea {
                    anchors.fill: button
                    cursorShape: Qt.PointingHandCursor
                    enabled: false
                }
            }

            DividerType {}

            DrawerType2 {
                id: configContentDrawer

                expandedHeight: root.height * 0.9

                parent: root
                anchors.fill: parent

                expandedStateContent: Item {
                    implicitHeight: configContentDrawer.expandedHeight

                    BackButtonType {
                        id: drawerBackButton

                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.topMargin: 16

                        backButtonFunction: function() {
                            configContentDrawer.closeTriggered()
                        }
                    }

                    ListViewType {
                        id: drawerListView

                        anchors.top: drawerBackButton.bottom
                        anchors.bottom: parent.bottom
                        anchors.right: parent.right
                        anchors.left: parent.left

                        header: ColumnLayout {
                            width: drawerListView.width

                            Header2Type {
                                Layout.fillWidth: true
                                Layout.topMargin: 16
                                Layout.leftMargin: 16
                                Layout.rightMargin: 16

                                headerText: qsTr("Connection options %1").arg(protocolName)
                            }
                        }

                        model: 1 // fake model to force the ListView to be created without a model

                        delegate: ColumnLayout {
                            width: drawerListView.width

                            TextArea {
                                id: configText

                                Layout.fillWidth: true
                                Layout.topMargin: 16
                                Layout.leftMargin: 16
                                Layout.rightMargin: 16

                                padding: 0
                                height: 24

                                color: AmneziaStyle.color.paleGray
                                selectionColor: AmneziaStyle.color.richBrown
                                selectedTextColor: AmneziaStyle.color.paleGray

                                font.pixelSize: 16
                                font.weight: Font.Medium
                                font.family: "PT Root UI VF"

                                text: rawConfig

                                wrapMode: Text.Wrap

                                background: Rectangle {
                                    color: AmneziaStyle.color.transparent
                                }
                            }
                        }
                    }
                }
            }
        }

        footer: ColumnLayout {
            width: listView.width

            LabelWithButtonType {
                id: removeButton

                width: parent.width

                visible: ServersModel.isProcessedServerHasWriteAccess()

                text: qsTr("Remove ") + ContainersModel.getProcessedContainerName()
                textColor: AmneziaStyle.color.vibrantRed

                clickedFunction: function() {
                    var headerText = qsTr("Remove %1 from server?").arg(ContainersModel.getProcessedContainerName())
                    var descriptionText = qsTr("All users with whom you shared a connection with will no longer be able to connect to it.")
                    var yesButtonText = qsTr("Continue")
                    var noButtonText = qsTr("Cancel")

                    var yesButtonFunction = function() {
                        PageController.goToPage(PageEnum.PageDeinstalling)
                        InstallController.removeProcessedContainer()
                    }
                    var noButtonFunction = function() {}

                    showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                }

                MouseArea {
                    anchors.fill: removeButton
                    cursorShape: Qt.PointingHandCursor
                    enabled: false
                }
            }

            DividerType {}
        }
    }
}
