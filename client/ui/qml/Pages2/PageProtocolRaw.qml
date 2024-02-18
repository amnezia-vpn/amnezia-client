import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ProtocolEnum 1.0
import ContainerEnum 1.0
import ContainerProps 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    ColumnLayout {
        id: header

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        anchors.topMargin: 20

        BackButtonType {
        }

        HeaderType {
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16

            headerText: ContainersModel.getCurrentlyProcessedContainerName() + qsTr(" settings")
        }
    }

    FlickableType {
        id: fl
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        contentHeight: content.height

        Column {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 32

            ListView {
                width: parent.width
                height: contentItem.height
                clip: true
                interactive: false
                model: ProtocolsModel

                delegate: Item {
                    implicitWidth: parent.width
                    implicitHeight: delegateContent.implicitHeight

                    ColumnLayout {
                        id: delegateContent

                        anchors.fill: parent

                        LabelWithButtonType {
                            id: button

                            Layout.fillWidth: true

                            text: qsTr("Show connection options")

                            clickedFunction: function() {
                                configContentDrawer.open()
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

                            expandedContent: Item {
                                implicitHeight: configContentDrawer.expandedHeight

                                BackButtonType {
                                    id: backButton

                                    anchors.top: parent.top
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.topMargin: 16

                                    backButtonFunction: function() {
                                        configContentDrawer.close()
                                    }
                                }

                                FlickableType {
                                    anchors.top: backButton.bottom
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.bottom: parent.bottom
                                    contentHeight: configContent.implicitHeight + configContent.anchors.topMargin + configContent.anchors.bottomMargin

                                    ColumnLayout {
                                        id: configContent

                                        anchors.fill: parent
                                        anchors.rightMargin: 16
                                        anchors.leftMargin: 16

                                        Header2Type {
                                            Layout.fillWidth: true
                                            Layout.topMargin: 16

                                            headerText: qsTr("Connection options %1").arg(protocolName)
                                        }

                                        TextArea {
                                            id: configText

                                            Layout.fillWidth: true
                                            Layout.topMargin: 16
                                            Layout.bottomMargin: 16

                                            padding: 0
                                            leftPadding: 0
                                            height: 24

                                            color: "#D7D8DB"
                                            selectionColor:  "#633303"
                                            selectedTextColor: "#D7D8DB"

                                            font.pixelSize: 16
                                            font.weight: Font.Medium
                                            font.family: "PT Root UI VF"

                                            text: rawConfig

                                            wrapMode: Text.Wrap

                                            background: Rectangle {
                                                color: "transparent"
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            LabelWithButtonType {
                id: removeButton

                width: parent.width

                visible: ServersModel.isProcessedServerHasWriteAccess()

                text: qsTr("Remove ") + ContainersModel.getCurrentlyProcessedContainerName()
                textColor: "#EB5757"

                clickedFunction: function() {
                    var headerText = qsTr("Remove %1 from server?").arg(ContainersModel.getCurrentlyProcessedContainerName())
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
