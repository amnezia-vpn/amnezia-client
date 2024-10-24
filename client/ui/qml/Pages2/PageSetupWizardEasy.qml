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
import "../Config"

PageType {
    id: root

    property bool isEasySetup: true

    SortFilterProxyModel {
        id: proxyContainersModel
        sourceModel: ContainersModel
        filters: [
            ValueFilter {
                roleName: "isEasySetupContainer"
                value: true
            }
        ]
        sorters: RoleSorter {
            roleName: "easySetupOrder"
            sortOrder: Qt.DescendingOrder
        }
    }

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20
    }

    FlickableType {
        id: fl
        anchors.top: backButton.bottom
        anchors.bottom: parent.bottom
        contentHeight: content.implicitHeight + setupLaterButton.anchors.bottomMargin

        Column {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.rightMargin: 16
            anchors.leftMargin: 16

            spacing: 16

            HeaderType {
                id: header

                implicitWidth: parent.width
                headerTextMaximumLineCount: 10

                headerText: qsTr("What is the level of internet control in your region?")
            }

            ButtonGroup {
                id: buttonGroup
            }

            ListView {
                id: containers
                width: parent.width
                height: containers.contentItem.height
                spacing: 16

                currentIndex: 1
                clip: true
                interactive: false
                model: proxyContainersModel

                property int dockerContainer
                property int containerDefaultPort
                property int containerDefaultTransportProto

                delegate: Item {
                    implicitWidth: containers.width
                    implicitHeight: delegateContent.implicitHeight

                    ColumnLayout {
                        id: delegateContent

                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right

                        CardType {
                            id: card

                            Layout.fillWidth: true

                            headerText: easySetupHeader
                            bodyText: easySetupDescription

                            ButtonGroup.group: buttonGroup

                            onClicked: function() {
                                isEasySetup = true
                                var defaultContainerProto =  ContainerProps.defaultProtocol(dockerContainer)

                                containers.dockerContainer = dockerContainer
                                containers.containerDefaultPort = ProtocolProps.getPortForInstall(defaultContainerProto)
                                containers.containerDefaultTransportProto = ProtocolProps.defaultTransportProto(defaultContainerProto)
                            }
                        }
                    }
                }

                Component.onCompleted: {
                    var item = containers.itemAtIndex(containers.currentIndex)
                    if (item !== null) {
                        var button = item.children[0].children[0]
                        button.checked = true
                        button.clicked()
                    }
                }
            }

            DividerType {
                implicitWidth: parent.width
            }

            CardType {
                implicitWidth: parent.width

                headerText: qsTr("Choose a VPN protocol")

                ButtonGroup.group: buttonGroup

                onClicked: function() {
                    isEasySetup = false
                }
            }

            BasicButtonType {
                id: continueButton

                implicitWidth: parent.width

                text: qsTr("Continue")
                
                parentFlickable: fl

                clickedFunc: function() {
                    if (root.isEasySetup) {
                        ContainersModel.setProcessedContainerIndex(containers.dockerContainer)
                        PageController.goToPage(PageEnum.PageSetupWizardInstalling)
                        InstallController.install(containers.dockerContainer,
                                                  containers.containerDefaultPort,
                                                  containers.containerDefaultTransportProto)
                    } else {
                        PageController.goToPage(PageEnum.PageSetupWizardProtocols)
                    }
                }
            }

            BasicButtonType {
                id: setupLaterButton

                implicitWidth: parent.width
                anchors.topMargin: 8
                anchors.bottomMargin: 24

                defaultColor: AmneziaStyle.color.transparent
                hoveredColor: AmneziaStyle.color.translucentWhite
                pressedColor: AmneziaStyle.color.sheerWhite
                disabledColor: AmneziaStyle.color.mutedGray
                textColor: AmneziaStyle.color.paleGray
                borderWidth: 1

                Keys.onTabPressed: lastItemTabClicked(focusItem)
                parentFlickable: fl

                visible: {
                    if (PageController.isTriggeredByConnectButton()) {
                        PageController.setTriggeredByConnectButton(false)
                        return false
                    }

                    return  true
                }

                text: qsTr("Skip setup")

                clickedFunc: function() {
                    PageController.goToPage(PageEnum.PageSetupWizardInstalling)
                    InstallController.addEmptyServer()
                }
            }
        }
    }
}
