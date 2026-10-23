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

    property int dockerContainer
    property int containerDefaultPort
    property int containerDefaultTransportProto

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

    ButtonGroup {
        id: buttonGroup
    }

    FlickableType {
        id: flickable

        anchors.top: parent.top
        anchors.bottom: bottomBar.top

        contentHeight: content.implicitHeight + 16

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 0

            NavigationBarType {
                Layout.fillWidth: true

                title: qsTr("Choose Installation Type")
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                spacing: 12

                Repeater {
                    model: proxyContainersModel

                    delegate: SelectableListItemType {
                        required property int index
                        required property int dockerContainer
                        required property string easySetupHeader
                        required property string easySetupDescription

                        Layout.fillWidth: true

                        showSeparator: index > 0

                        text: easySetupHeader
                        subtitle: easySetupDescription

                        ButtonGroup.group: buttonGroup

                        onClicked: {
                            root.isEasySetup = true
                            checked = true
                            var defaultContainerProto = ContainerProps.defaultProtocol(dockerContainer)

                            root.dockerContainer = dockerContainer
                            root.containerDefaultPort = InstallController.getPortForInstall(defaultContainerProto)
                            root.containerDefaultTransportProto = InstallController.defaultTransportProto(defaultContainerProto)
                        }

                        Component.onCompleted: {
                            if (index === 0) {
                                checked = true
                                clicked()
                            }
                        }
                    }
                }

                SelectableListItemType {
                    Layout.fillWidth: true

                    text: qsTr("Manual")
                    subtitle: qsTr("Choose a VPN protocol")

                    ButtonGroup.group: buttonGroup

                    onClicked: {
                        root.isEasySetup = false
                        checked = true
                    }
                }
            }
        }
    }

    BottomBarType {
        id: bottomBar

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        filled: true

        ButtonType {
            id: continueButton

            Layout.fillWidth: true

            text: qsTr("Continue")

            onClicked: {
                if (root.isEasySetup) {
                    ServersUiController.processedContainerIndex = root.dockerContainer
                    PageController.goToPage(PageEnum.PageSetupWizardInstalling)
                    InstallController.install(root.dockerContainer,
                                              root.containerDefaultPort,
                                              root.containerDefaultTransportProto,
                                              ServersUiController.processedServerId)
                } else {
                    PageController.goToPage(PageEnum.PageSetupWizardProtocols)
                }
            }
        }

        ButtonType {
            id: setupLaterButton

            Layout.fillWidth: true

            variant: ButtonType.Secondary

            visible: {
                if (PageController.isTriggeredByConnectButton()) {
                    PageController.setTriggeredByConnectButton(false)
                    return false
                }

                return true
            }

            text: qsTr("Skip setup")

            onClicked: {
                PageController.goToPage(PageEnum.PageSetupWizardInstalling)
                InstallController.addEmptyServer()
            }
        }
    }
}
