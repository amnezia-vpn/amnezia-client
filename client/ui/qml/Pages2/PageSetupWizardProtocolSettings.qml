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

                    property alias port:port

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

                            headerText: qsTr("Installing %1").arg(name)
                            descriptionText: description
                        }

                        BasicButtonType {
                            id: showDetailsButton

                            Layout.topMargin: 16
                            Layout.leftMargin: -8

                            implicitHeight: 32

                            defaultColor: AmneziaStyle.color.transparent
                            hoveredColor: AmneziaStyle.color.translucentWhite
                            pressedColor: AmneziaStyle.color.sheerWhite
                            disabledColor: AmneziaStyle.color.mutedGray
                            textColor: AmneziaStyle.color.goldenApricot

                            text: qsTr("More detailed")
                            KeyNavigation.tab: transportProtoSelector

                            clickedFunc: function() {
                                showDetailsDrawer.open()
                            }
                        }

                        DrawerType2 {
                            id: showDetailsDrawer
                            parent: root
                            onClosed: {
                                if (!GC.isMobile()) {
                                    // defaultActiveFocusItem.forceActiveFocus()
                                }
                            }

                            anchors.fill: parent
                            expandedHeight: parent.height * 0.9
                            expandedStateContent: Item {
                                Connections {
                                    target: showDetailsDrawer
                                    enabled: !GC.isMobile()
                                    function onOpened() {
                                        focusItem2.forceActiveFocus()
                                    }
                                }

                                implicitHeight: showDetailsDrawer.expandedHeight

                                // Item {
                                //     id: focusItem2
                                //     KeyNavigation.tab: showDetailsBackButton
                                //     onFocusChanged: {
                                //         if (focusItem2.activeFocus) {
                                //             fl.contentY = 0
                                //         }
                                //     }
                                // }

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
                                    id: fl
                                    anchors.top: showDetailsBackButton.bottom
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.bottom: parent.bottom
                                    contentHeight: {
                                        var emptySpaceHeight = parent.height - showDetailsBackButton.implicitHeight - showDetailsBackButton.anchors.topMargin
                                        return (showDetailsDrawerContent.height > emptySpaceHeight) ?
                                                    showDetailsDrawerContent.height : emptySpaceHeight
                                    }

                                    ColumnLayout {
                                        id: showDetailsDrawerContent

                                        anchors.top: parent.top
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        anchors.rightMargin: 16
                                        anchors.leftMargin: 16

                                        Header2Type {
                                            id: showDetailsDrawerHeader
                                            Layout.fillWidth: true
                                            Layout.topMargin: 16

                                            headerText: name
                                        }

                                        ParagraphTextType {
                                            Layout.fillWidth: true
                                            Layout.topMargin: 16
                                            Layout.bottomMargin: 16

                                            text: detailedDescription
                                            textFormat: Text.MarkdownText
                                        }

                                        Rectangle {
                                            Layout.fillHeight: true
                                            color: AmneziaStyle.color.transparent
                                        }

                                        BasicButtonType {
                                            id: showDetailsCloseButton
                                            Layout.fillWidth: true
                                            Layout.bottomMargin: 32
                                            parentFlickable: fl

                                            text: qsTr("Close")
                                            Keys.onTabPressed: lastItemTabClicked(focusItem2)

											clickedFunc: function()  {
                                                showDetailsDrawer.close()
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        ParagraphTextType {
                            id: transportProtoHeader

                            Layout.topMargin: 16

                            text: qsTr("Network protocol")
                        }

                        TransportProtoSelector {
                            id: transportProtoSelector

                            Layout.fillWidth: true
                            rootWidth: root.width

                            // KeyNavigation.tab: (port.visible && port.enabled) ? port.textField : installButton
                        }

                        TextFieldWithHeaderType {
                            id: port

                            Layout.fillWidth: true
                            Layout.topMargin: 16

                            headerText: qsTr("Port")
                            textField.maximumLength: 5
                            textField.validator: IntValidator { bottom: 1; top: 65535 }
                        }

                        Rectangle {
                            Layout.fillHeight: true
                            color: AmneziaStyle.color.transparent
                        }

                        BasicButtonType {
                            id: installButton

                            Layout.fillWidth: true
                            Layout.bottomMargin: 32

                            text: qsTr("Install")

                            Keys.onTabPressed: lastItemTabClicked(focusItem)

                            clickedFunc: function() {
                                if (!port.textField.acceptableInput &&
                                        ContainerProps.containerTypeToString(dockerContainer) !== "torwebsite" &&
                                        ContainerProps.containerTypeToString(dockerContainer) !== "ikev2") {
                                    port.errorText = qsTr("The port must be in the range of 1 to 65535")
                                    return
                                }

                                PageController.goToPage(PageEnum.PageSetupWizardInstalling);
                                InstallController.install(dockerContainer, port.textFieldText, transportProtoSelector.currentIndex)
                            }
                        }

                        Component.onCompleted: {
                            var defaultContainerProto =  ContainerProps.defaultProtocol(dockerContainer)

                            if (ProtocolProps.defaultPort(defaultContainerProto) < 0) {
                                port.visible = false
                            } else {
                                port.textFieldText = ProtocolProps.getPortForInstall(defaultContainerProto)
                            }
                            transportProtoSelector.currentIndex = ProtocolProps.defaultTransportProto(defaultContainerProto)

                            port.enabled = ProtocolProps.defaultPortChangeable(defaultContainerProto)
                            var protocolSelectorVisible = ProtocolProps.defaultTransportProtoChangeable(defaultContainerProto)
                            transportProtoSelector.visible = protocolSelectorVisible
                            transportProtoHeader.visible = protocolSelectorVisible

                            // if (port.visible && port.enabled)
                            //     defaultActiveFocusItem = port.textField
                            // else
                            //     defaultActiveFocusItem = focusItem
                        }
                    }
                }
            }
        }
    }
}
