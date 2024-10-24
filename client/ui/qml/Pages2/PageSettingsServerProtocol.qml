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

    property bool isClearCacheVisible: ServersModel.isProcessedServerHasWriteAccess() && !ContainersModel.isServiceContainer(ContainersModel.getProcessedContainerIndex())

    ColumnLayout {
        id: header

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        anchors.topMargin: 20

        BackButtonType {
            id: backButton
        }

        HeaderType {
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.bottomMargin: 32

            headerText: ContainersModel.getProcessedContainerName() + qsTr(" settings")
        }

        ListView {
            id: protocols
            Layout.fillWidth: true
            height: protocols.contentItem.height
            clip: true
            interactive: true

            property bool isFocusable: true

            Keys.onTabPressed: {
                FocusController.nextKeyTabItem()
            }

            Keys.onBacktabPressed: {
                FocusController.previousKeyTabItem()
            }

            Keys.onUpPressed: {
                FocusController.nextKeyUpItem()
            }

            Keys.onDownPressed: {
                FocusController.nextKeyDownItem()
            }

            Keys.onLeftPressed: {
                FocusController.nextKeyLeftItem()
            }

            Keys.onRightPressed: {
                FocusController.nextKeyRightItem()
            }

            model: ProtocolsModel

            delegate: Item {
                implicitWidth: protocols.width
                implicitHeight: delegateContent.implicitHeight

                ColumnLayout {
                    id: delegateContent

                    anchors.fill: parent

                    property bool isClientSettingsVisible: protocolIndex === ProtocolEnum.WireGuard || protocolIndex === ProtocolEnum.Awg
                    property bool isServerSettingsVisible: ServersModel.isProcessedServerHasWriteAccess()

                    LabelWithButtonType {
                        id: clientSettings

                        Layout.fillWidth: true

                        text: protocolName + qsTr(" connection settings")
                        rightImageSource: "qrc:/images/controls/chevron-right.svg"
                        visible: delegateContent.isClientSettingsVisible

                        clickedFunction: function() {
                            if (isClientProtocolExists) {
                                switch (protocolIndex) {
                                case ProtocolEnum.WireGuard: WireGuardConfigModel.updateModel(ProtocolsModel.getConfig()); break;
                                case ProtocolEnum.Awg: AwgConfigModel.updateModel(ProtocolsModel.getConfig()); break;
                                }
                                PageController.goToPage(clientProtocolPage);
                            } else {
                                PageController.showNotificationMessage(qsTr("Click the \"connect\" button to create a connection configuration"))
                            }
                        }

                        MouseArea {
                            anchors.fill: clientSettings
                            cursorShape: Qt.PointingHandCursor
                            enabled: false
                        }
                    }

                    DividerType {
                        visible: delegateContent.isClientSettingsVisible
                    }

                    LabelWithButtonType {
                        id: serverSettings

                        Layout.fillWidth: true

                        text: protocolName + qsTr(" server settings")
                        rightImageSource: "qrc:/images/controls/chevron-right.svg"
                        visible: delegateContent.isServerSettingsVisible

                        clickedFunction: function() {
                            switch (protocolIndex) {
                            case ProtocolEnum.OpenVpn: OpenVpnConfigModel.updateModel(ProtocolsModel.getConfig()); break;
                            case ProtocolEnum.ShadowSocks: ShadowSocksConfigModel.updateModel(ProtocolsModel.getConfig()); break;
                            case ProtocolEnum.Cloak: CloakConfigModel.updateModel(ProtocolsModel.getConfig()); break;
                            case ProtocolEnum.WireGuard: WireGuardConfigModel.updateModel(ProtocolsModel.getConfig()); break;
                            case ProtocolEnum.Awg: AwgConfigModel.updateModel(ProtocolsModel.getConfig()); break;
                            case ProtocolEnum.Xray: XrayConfigModel.updateModel(ProtocolsModel.getConfig()); break;
                            case ProtocolEnum.Sftp: SftpConfigModel.updateModel(ProtocolsModel.getConfig()); break;
                            case ProtocolEnum.Ipsec: Ikev2ConfigModel.updateModel(ProtocolsModel.getConfig()); break;
                            case ProtocolEnum.Socks5Proxy: Socks5ProxyConfigModel.updateModel(ProtocolsModel.getConfig()); break;
                            }
                            PageController.goToPage(serverProtocolPage);
                        }

                        MouseArea {
                            anchors.fill: serverSettings
                            cursorShape: Qt.PointingHandCursor
                            enabled: false
                        }
                    }

                    DividerType {
                        visible: delegateContent.isServerSettingsVisible
                    }
                }
            }

            footer: ColumnLayout {
                width: header.width

                LabelWithButtonType {
                    id: clearCacheButton

                    Layout.fillWidth: true

                    visible: root.isClearCacheVisible

                    text: qsTr("Clear profile")

                    clickedFunction: function() {
                        var headerText = qsTr("Clear %1 profile?").arg(ContainersModel.getProcessedContainerName())
                        var descriptionText = qsTr("The connection configuration will be deleted for this device only")
                        var yesButtonText = qsTr("Continue")
                        var noButtonText = qsTr("Cancel")

                        var yesButtonFunction = function() {
                            if (ConnectionController.isConnected && ServersModel.getDefaultServerData("defaultContainer") === ContainersModel.getProcessedContainerIndex()) {
                                var message = qsTr("Unable to clear %1 profile while there is an active connection").arg(ContainersModel.getProcessedContainerName())
                                PageController.showNotificationMessage(message)
                                return
                            }

                            PageController.showBusyIndicator(true)
                            InstallController.clearCachedProfile()
                            PageController.showBusyIndicator(false)
                        }
                        var noButtonFunction = function() {
                            // if (!GC.isMobile()) {
                            //     focusItem.forceActiveFocus()
                            // }
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

                    visible: root.isClearCacheVisible
                }

                LabelWithButtonType {
                    id: removeButton

                    Layout.fillWidth: true

                    visible: ServersModel.isProcessedServerHasWriteAccess()

                    text: qsTr("Remove ")
                    textColor: AmneziaStyle.color.vibrantRed

                    clickedFunction: function() {
                        var headerText = qsTr("Remove %1 from server?").arg(ContainersModel.getProcessedContainerName())
                        var descriptionText = qsTr("All users with whom you shared a connection will no longer be able to connect to it.")
                        var yesButtonText = qsTr("Continue")
                        var noButtonText = qsTr("Cancel")

                        var yesButtonFunction = function() {
                            if (ServersModel.isDefaultServerCurrentlyProcessed() && ConnectionController.isConnected
                                    && ServersModel.getDefaultServerData("defaultContainer") === ContainersModel.getProcessedContainerIndex()) {
                                PageController.showNotificationMessage(qsTr("Cannot remove active container"))
                            } else
                            {
                                PageController.goToPage(PageEnum.PageDeinstalling)
                                InstallController.removeProcessedContainer()
                            }
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
}

