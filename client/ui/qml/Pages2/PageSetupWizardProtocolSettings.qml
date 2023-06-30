import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ContainerProps 1.0
import ProtocolProps 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    SortFilterProxyModel {
        id: proxyContainersModel
        sourceModel: ContainersModel
        filters: [
            ValueFilter {
                roleName: "isCurrentlyProcessed"
                value: true
            }
        ]
    }

    FlickableType {
        anchors.fill: parent
        contentHeight: content.height

        Column {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            ListView {
                id: processedContainerListView
                width: parent.width
                height: contentItem.height
                currentIndex: -1
                clip: true
                interactive: false
                model: proxyContainersModel

                delegate: Item {
                    implicitWidth: processedContainerListView.width
                    implicitHeight: (delegateContent.implicitHeight > root.height) ? delegateContent.implicitHeight : root.height

                    ColumnLayout {
                        id: delegateContent

                        anchors.fill: parent
                        anchors.rightMargin: 16
                        anchors.leftMargin: 16

                        BackButtonType {
                            id: backButton

                            Layout.topMargin: 20
                            Layout.rightMargin: -16
                            Layout.leftMargin: -16
                        }

                        HeaderType {
                            id: header

                            Layout.fillWidth: true

                            headerText: qsTr("Installing ") + name
                            descriptionText: qsTr("protocol description")
                        }

                        BasicButtonType {
                            id: showDetailsButton

                            Layout.topMargin: 16
                            Layout.leftMargin: -8

                            implicitHeight: 32

                            defaultColor: "transparent"
                            hoveredColor: Qt.rgba(1, 1, 1, 0.08)
                            pressedColor: Qt.rgba(1, 1, 1, 0.12)
                            disabledColor: "#878B91"
                            textColor: "#FBB26A"

                            text: qsTr("More detailed")

                            onClicked: {
                                showDetailsDrawer.open()
                            }
                        }

                        DrawerType {
                            id: showDetailsDrawer

                            width: parent.width
                            height: parent.height * 0.9

                            BackButtonType {
                                id: showDetailsBackButton

                                anchors.top: parent.top
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.topMargin: 16

                                backButtonFunction: function() {
                                    showDetailsDrawer.close()
                                }
                            }

                            FlickableType {
                                anchors.top: showDetailsBackButton.bottom
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.bottom: parent.bottom
                                contentHeight: {
                                    var emptySpaceHeight = parent.height - showDetailsBackButton.implicitHeight - showDetailsBackButton.anchors.topMargin

                                    return (showDetailsDrawerContent.implicitHeight > emptySpaceHeight) ?
                                                showDetailsDrawerContent.implicitHeight : emptySpaceHeight
                                }

                                ColumnLayout {
                                    id: showDetailsDrawerContent

                                    anchors.fill: parent
                                    anchors.rightMargin: 16
                                    anchors.leftMargin: 16

                                    Header2Type {
                                        id: showDetailsDrawerHeader
                                        Layout.fillWidth: true
                                        Layout.topMargin: 16

                                        headerText: name
                                    }

                                    TextField {
                                        Layout.fillWidth: true
                                        Layout.topMargin: 16
                                        Layout.bottomMargin: 16

                                        padding: 0
                                        leftPadding: 0
                                        height: 24

                                        color: "#D7D8DB"

                                        font.pixelSize: 16
                                        font.weight: Font.Medium
                                        font.family: "PT Root UI VF"

                                        text: qsTr("detailed protocol description")

                                        wrapMode: Text.WordWrap

                                        readOnly: true
                                        background: Rectangle {
                                            anchors.fill: parent
                                            color: "transparent"
                                        }
                                    }

                                    Rectangle {
                                        Layout.fillHeight: true
                                        color: "transparent"
                                    }

                                    BasicButtonType {
                                        Layout.fillWidth: true
                                        Layout.bottomMargin: 32

                                        text: qsTr("Close")

                                        onClicked: function() {
                                            showDetailsDrawer.close()
                                        }
                                    }
                                }
                            }
                        }

                        ParagraphTextType {
                            id: transportProtoHeader

                            Layout.topMargin: 16

                            text: "Network protocol"
                        }

                        TransportProtoSelector {
                            id: transportProtoSelector

                            Layout.fillWidth: true
                            rootWidth: root.width
                        }

                        TextFieldWithHeaderType {
                            id: port

                            Layout.fillWidth: true
                            Layout.topMargin: 16

                            headerText: "Port"
                        }

                        Rectangle {
                            Layout.fillHeight: true
                            color: "transparent"
                        }

                        BasicButtonType {
                            id: installButton

                            Layout.fillWidth: true
                            Layout.bottomMargin: 32

                            text: qsTr("Установить")

                            onClicked: function() {
                                goToPage(PageEnum.PageSetupWizardInstalling);
                                InstallController.install(dockerContainer, port.textFieldText, transportProtoSelector.currentIndex)
                            }
                        }

                        Component.onCompleted: {
                            //todo move to protocols model?
                            var defaultContainerProto =  ContainerProps.defaultProtocol(dockerContainer)

                            if (ProtocolProps.defaultPort(defaultContainerProto) < 0) {
                                port.visible = false
                            } else {
                                port.textFieldText = ProtocolProps.defaultPort(defaultContainerProto)
                            }
                            transportProtoSelector.currentIndex = ProtocolProps.defaultTransportProto(defaultContainerProto)

                            port.enabled = ProtocolProps.defaultPortChangeable(defaultContainerProto)
                            var protocolSelectorVisible = ProtocolProps.defaultTransportProtoChangeable(defaultContainerProto)
                            transportProtoSelector.visible = protocolSelectorVisible
                            transportProtoHeader.visible = protocolSelectorVisible
                        }
                    }
                }
            }
        }
    }
}
