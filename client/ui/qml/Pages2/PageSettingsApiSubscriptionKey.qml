import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import Qt.labs.platform 1.1

import QtCore

import SortFilterProxyModel 0.2

import PageEnum 1.0
import Style 1.0

import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    property var processedServer

    Connections {
        target: ServersUiController

        function onProcessedServerIdChanged() {
            root.processedServer = proxyServersModel.get(0)
        }
    }

    Connections {
        target: ServersModel

        function onModelReset() {
            root.processedServer = proxyServersModel.get(0)
        }
    }

    SortFilterProxyModel {
        id: proxyServersModel

        sourceModel: ServersModel
        filters: [
            ValueFilter {
                roleName: "serverId"
                value: ServersUiController.processedServerId
            }
        ]

        Component.onCompleted: {
            root.processedServer = proxyServersModel.get(0)
        }
    }

    Component.onCompleted: {
        PageController.showBusyIndicator(true)
        SubscriptionUiController.prepareVpnKeyExport(ServersUiController.processedServerId)
        PageController.showBusyIndicator(false)
    }

    FlickableType {
        id: flickable

        anchors.fill: parent

        contentHeight: layout.implicitHeight

        ColumnLayout {
            id: layout

            width: flickable.width

            spacing: 0

            NavigationBarType {
                Layout.fillWidth: true

                title: root.processedServer ? qsTr(root.processedServer.name + "\nsubscription key") : ""
            }

            ButtonType {
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Copy key")
                leftImageSource: "qrc:/images/controls/copy.svg"

                onClicked: {
                    SubscriptionUiController.copyVpnKeyToClipboard()
                    PageController.showNotificationMessage(qsTr("Copied"))
                }
            }

            ButtonType {
                variant: ButtonType.Secondary

                Layout.fillWidth: true
                Layout.topMargin: 12
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Save key as a file")
                leftImageSource: "qrc:/images/controls/share-2.svg"

                onClicked: {
                    var fileName = GC.isMobile()
                        ? root.processedServer.name.toLowerCase().replace(/\s+/g, "_") + "_key.vpn"
                        : SystemController.getFileName(
                            qsTr("Save AmneziaVPN config"),
                            qsTr("Config files (*.vpn)"),
                            StandardPaths.standardLocations(StandardPaths.DocumentsLocation) + "/" + root.processedServer.name.toLowerCase().replace(/\s+/g, "_") + "_key",
                            true,
                            ".vpn"
                        )

                    if (fileName !== "") {
                        PageController.showBusyIndicator(true)
                        let ok = SubscriptionUiController.exportVpnKey(ServersUiController.processedServerId, fileName)
                        PageController.showBusyIndicator(false)
                        if (ok) {
                            PageController.showNotificationMessage(qsTr("Config file saved"))
                        }
                    }
                }
            }

            ButtonType {
                variant: ButtonType.Secondary

                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Show key text")
                leftImageSource: "qrc:/images/controls/eye.svg"

                onClicked: {
                    PageController.showBusyIndicator(true)
                    SubscriptionUiController.prepareVpnKeyExport(ServersUiController.processedServerId)
                    PageController.showBusyIndicator(false)
                    vpnKeyDrawer.open()
                }
            }

            QrCodeCard {
                Layout.fillWidth: true
                Layout.maximumWidth: 360
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                visible: SubscriptionUiController.qrCodesCount > 0

                source: SubscriptionUiController.qrCodesCount > 0 && SubscriptionUiController.qrCodes[0] ? SubscriptionUiController.qrCodes[0] : ""
            }

            SmallTextType {
                Layout.fillWidth: true
                Layout.topMargin: 12
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                visible: SubscriptionUiController.qrCodesCount > 0

                horizontalAlignment: Text.AlignHCenter
                color: AmneziaStyle.color.textTertiary
                text: qsTr("To read the QR code in the Amnezia app, tap + in the main menu → 'QR code'")
            }

            Item {
                Layout.preferredHeight: 16 + PageController.safeAreaBottomMargin
            }
        }
    }

    ConfigContentSheet {
        id: vpnKeyDrawer

        headerText: root.processedServer ? qsTr(root.processedServer.name + " Subscription key") : ""
        contentText: SubscriptionUiController.vpnKey
    }
}
