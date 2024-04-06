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

    defaultActiveFocusItem: focusItem

    Item {
        id: focusItem
        KeyNavigation.tab: backButton
    }

    ColumnLayout {
        id: header

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        anchors.topMargin: 20

        BackButtonType {
            id: backButton
            KeyNavigation.tab: clearCacheButton
        }

        HeaderType {
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.bottomMargin: 32

            headerText: ContainersModel.getProcessedContainerName() + qsTr(" settings")
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
                id: protocols
                Layout.fillWidth: true
                height: protocols.contentItem.height
                clip: true
                interactive: true
                model: ProtocolsModel

                activeFocusOnTab: true
                Keys.onTabPressed: {
                    if (currentIndex < this.count - 1) {
                        this.incrementCurrentIndex()
                    } else {
                        this.currentIndex = 0
                        removeButton.forceActiveFocus()
                    }
                }

                onVisibleChanged: {
                    if (visible) {
                        this.currentIndex = 0
                    }
                }

                delegate: Item {
                    implicitWidth: protocols.width
                    implicitHeight: delegateContent.implicitHeight

                    onFocusChanged: {
                        if (focus) {
                            button.rightButton.forceActiveFocus()
                        }
                    }

                    ColumnLayout {
                        id: delegateContent

                        anchors.fill: parent

                        LabelWithButtonType {
                            id: button

                            Layout.fillWidth: true

                            text: protocolName
                            rightImageSource: "qrc:/images/controls/chevron-right.svg"

                            clickedFunction: function() {
                                switch (protocolIndex) {
                                case ProtocolEnum.OpenVpn: OpenVpnConfigModel.updateModel(ProtocolsModel.getConfig()); break;
                                case ProtocolEnum.ShadowSocks: ShadowSocksConfigModel.updateModel(ProtocolsModel.getConfig()); break;
                                case ProtocolEnum.Cloak: CloakConfigModel.updateModel(ProtocolsModel.getConfig()); break;
                                case ProtocolEnum.WireGuard: WireGuardConfigModel.updateModel(ProtocolsModel.getConfig()); break;
                                case ProtocolEnum.Awg: AwgConfigModel.updateModel(ProtocolsModel.getConfig()); break;
                                case ProtocolEnum.Xray: XrayConfigModel.updateModel(ProtocolsModel.getConfig()); break;
                                case ProtocolEnum.Ipsec: Ikev2ConfigModel.updateModel(ProtocolsModel.getConfig()); break;
                                }
                                PageController.goToPage(protocolPage);
                            }

                            MouseArea {
                                anchors.fill: button
                                cursorShape: Qt.PointingHandCursor
                                enabled: false
                            }
                        }

                        DividerType {}
                    }
                }
            }

            LabelWithButtonType {
                id: clearCacheButton

                Layout.fillWidth: true

                visible: ServersModel.isProcessedServerHasWriteAccess()
                KeyNavigation.tab: removeButton

                text: qsTr("Clear %1 profile").arg(ContainersModel.getProcessedContainerName())

                clickedFunction: function() {
                    var headerText = qsTr("Clear %1 profile?").arg(ContainersModel.getProcessedContainerName())
                    var descriptionText = qsTr("")
                    var yesButtonText = qsTr("Continue")
                    var noButtonText = qsTr("Cancel")

                    var yesButtonFunction = function() {
                        PageController.showBusyIndicator(true)
                        InstallController.clearCachedProfile()
                        PageController.showBusyIndicator(false)
                    }
                    var noButtonFunction = function() {
                        if (!GC.isMobile()) {
                            focusItem.forceActiveFocus()
                        }
                    }

                    showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                }

                MouseArea {
                    anchors.fill: clearCacheButton
                    cursorShape: Qt.PointingHandCursor
                    enabled: false
                }
            }

            DividerType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                visible: ServersModel.isProcessedServerHasWriteAccess()
            }

            LabelWithButtonType {
                id: removeButton

                Layout.fillWidth: true

                visible: ServersModel.isProcessedServerHasWriteAccess()

                text: qsTr("Remove ") + ContainersModel.getProcessedContainerName()
                textColor: "#EB5757"

                Keys.onTabPressed: lastItemTabClicked(focusItem)

                clickedFunction: function() {
                    var headerText = qsTr("Remove %1 from server?").arg(ContainersModel.getProcessedContainerName())
                    var descriptionText = qsTr("All users with whom you shared a connection will no longer be able to connect to it.")
                    var yesButtonText = qsTr("Continue")
                    var noButtonText = qsTr("Cancel")

                    var yesButtonFunction = function() {
                        PageController.goToPage(PageEnum.PageDeinstalling)
                        InstallController.removeProcessedContainer()
                    }
                    var noButtonFunction = function() {
                        if (!GC.isMobile()) {
                            focusItem.forceActiveFocus()
                        }
                    }

                    showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                }

                MouseArea {
                    anchors.fill: removeButton
                    cursorShape: Qt.PointingHandCursor
                    enabled: false
                }
            }

            DividerType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                visible: ServersModel.isProcessedServerHasWriteAccess()
            }
        }

    }
}
