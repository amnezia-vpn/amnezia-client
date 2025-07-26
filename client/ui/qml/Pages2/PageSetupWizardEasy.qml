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

        onActiveFocusChanged: {
            if(backButton.enabled && backButton.activeFocus) {
                listView.positionViewAtBeginning()
            }
        }
    }

    ButtonGroup {
        id: buttonGroup
    }

    ListViewType {
        id: listView

        property int dockerContainer
        property int containerDefaultPort
        property int containerDefaultTransportProto

        anchors.top: backButton.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        spacing: 16

        header: ColumnLayout {
            width: listView.width

            spacing: 16

            BaseHeaderType {
                id: header

                Layout.fillWidth: true
                Layout.rightMargin: 16
                Layout.leftMargin: 16
                Layout.bottomMargin: 16

                headerTextMaximumLineCount: 10

                headerText: qsTr("Choose Installation Type")
            }
        }

        model: proxyContainersModel
        currentIndex: 0

        delegate: ColumnLayout {
            width: listView.width

            CardType {
                id: card

                Layout.fillWidth: true
                Layout.rightMargin: 16
                Layout.leftMargin: 16
                Layout.bottomMargin: 16

                headerText: easySetupHeader
                bodyText: easySetupDescription

                ButtonGroup.group: buttonGroup

                onClicked: function() {
                    isEasySetup = true
                    var defaultContainerProto =  ContainerProps.defaultProtocol(dockerContainer)

                    listView.dockerContainer = dockerContainer
                    listView.containerDefaultPort = ProtocolProps.getPortForInstall(defaultContainerProto)
                    listView.containerDefaultTransportProto = ProtocolProps.defaultTransportProto(defaultContainerProto)
                }

                Keys.onReturnPressed: this.clicked()
                Keys.onEnterPressed: this.clicked()
            }
        }

        footer: ColumnLayout {
            width: listView.width
            spacing: 16

            DividerType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
            }

            CardType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("Manual")
                bodyText: qsTr("Choose a VPN protocol")

                ButtonGroup.group: buttonGroup

                onClicked: function() {
                    isEasySetup = false
                    checked = true
                }

                Keys.onEnterPressed: this.clicked()
                Keys.onReturnPressed: this.clicked()
            }

            BasicButtonType {
                id: continueButton

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Continue")

                clickedFunc: function() {
                    if (root.isEasySetup) {
                        ContainersModel.setProcessedContainerIndex(listView.dockerContainer)
                        PageController.goToPage(PageEnum.PageSetupWizardInstalling)
                        InstallController.install(listView.dockerContainer,
                                                  listView.containerDefaultPort,
                                                  listView.containerDefaultTransportProto)
                    } else {
                        PageController.goToPage(PageEnum.PageSetupWizardProtocols)
                    }
                }
            }

            BasicButtonType {
                id: setupLaterButton

                Layout.fillWidth: true
                Layout.topMargin: 8
                Layout.bottomMargin: 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                defaultColor: AmneziaStyle.color.transparent
                hoveredColor: AmneziaStyle.color.translucentWhite
                pressedColor: AmneziaStyle.color.sheerWhite
                disabledColor: AmneziaStyle.color.mutedGray
                textColor: AmneziaStyle.color.paleGray
                borderWidth: 1

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

        Component.onCompleted: {
            var item = listView.itemAtIndex(listView.currentIndex)
            if (item !== null) {
                var button = item.children[0]
                button.checked = true
                button.clicked()
            }
        }
    }
}

