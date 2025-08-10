import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ContainerProps 1.0
import ProtocolProps 1.0
import Style 1.0

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

        currentIndex: -1

        model: proxyContainersModel

        delegate: ColumnLayout {
            width: listView.width

            BaseHeaderType {
                id: header

                Layout.fillWidth: true
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                headerText: qsTr("Installing %1").arg(name)
                descriptionText: description
            }

            BasicButtonType {
                id: showDetailsButton

                Layout.topMargin: 16
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                implicitHeight: 32

                defaultColor: AmneziaStyle.color.transparent
                hoveredColor: AmneziaStyle.color.translucentWhite
                pressedColor: AmneziaStyle.color.sheerWhite
                disabledColor: AmneziaStyle.color.mutedGray
                textColor: AmneziaStyle.color.goldenApricot

                text: qsTr("More detailed")

                clickedFunc: function() {
                    showDetailsDrawer.openTriggered()
                }
            }

            DrawerType2 {
                id: showDetailsDrawer
                parent: root

                anchors.fill: parent
                expandedHeight: parent.height * 0.9
                expandedStateContent: Item {
                    implicitHeight: showDetailsDrawer.expandedHeight

                    BackButtonType {
                        id: showDetailsBackButton

                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.topMargin: 16

                        backButtonFunction: function() {
                            showDetailsDrawer.closeTriggered()
                        }
                    }

                    ListViewType {
                        id: showDetailsListView

                        anchors.top: showDetailsBackButton.bottom
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom

                        header: ColumnLayout {
                            width: showDetailsListView.width

                            Header2Type {
                                id: showDetailsDrawerHeader

                                Layout.fillWidth: true
                                Layout.topMargin: 16
                                Layout.rightMargin: 16
                                Layout.leftMargin: 16

                                headerText: name
                            }
                        }

                        model: 1 // fake model to force the ListView to be created without a model

                        delegate: ColumnLayout {
                            width: showDetailsListView.width

                            ParagraphTextType {
                                Layout.fillWidth: true
                                Layout.topMargin: 16
                                Layout.bottomMargin: 16
                                Layout.leftMargin: 16
                                Layout.rightMargin: 16

                                text: detailedDescription
                                textFormat: Text.MarkdownText
                            }

                            Rectangle {
                                Layout.fillHeight: true
                                Layout.leftMargin: 16
                                Layout.rightMargin: 16

                                color: AmneziaStyle.color.transparent
                            }
                        }

                        footer: ColumnLayout {
                            width: showDetailsListView.width

                            BasicButtonType {
                                id: showDetailsCloseButton
                                Layout.fillWidth: true
                                Layout.bottomMargin: 32
                                Layout.leftMargin: 16
                                Layout.rightMargin: 16

                                text: qsTr("Close")

                                clickedFunc: function()  {
                                    showDetailsDrawer.closeTriggered()
                                }
                            }
                        }
                    }
                }
            }

            ParagraphTextType {
                id: transportProtoHeader

                Layout.topMargin: 16
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                text: qsTr("Network protocol")
            }

            TransportProtoSelector {
                id: transportProtoSelector

                Layout.fillWidth: true
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                rootWidth: root.width
            }

            TextFieldWithHeaderType {
                id: port

                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                headerText: qsTr("Port")
                textField.maximumLength: 5
                textField.validator: IntValidator { bottom: 1; top: 65535 }
            }

            Rectangle {
                Layout.fillHeight: true
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                color: AmneziaStyle.color.transparent
            }

            BasicButtonType {
                id: installButton

                Layout.fillWidth: true
                Layout.bottomMargin: 32
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                text: qsTr("Install")

                clickedFunc: function() {
                    if (!port.textField.acceptableInput &&
                            ContainerProps.containerTypeToString(dockerContainer) !== "torwebsite" &&
                            ContainerProps.containerTypeToString(dockerContainer) !== "ikev2") {
                        port.errorText = qsTr("The port must be in the range of 1 to 65535")
                        return
                    }

                    PageController.goToPage(PageEnum.PageSetupWizardInstalling);
                    InstallController.install(dockerContainer, port.textField.text, transportProtoSelector.currentIndex)
                }
            }

            Component.onCompleted: {
                var defaultContainerProto =  ContainerProps.defaultProtocol(dockerContainer)

                if (ProtocolProps.defaultPort(defaultContainerProto) < 0) {
                    port.visible = false
                } else {
                    port.textField.text = ProtocolProps.getPortForInstall(defaultContainerProto)
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
