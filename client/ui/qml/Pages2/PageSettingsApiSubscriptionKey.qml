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
        target: ServersModel

        function onProcessedServerChanged() {
            root.processedServer = proxyServersModel.get(0)
        }
    }

    SortFilterProxyModel {
        id: proxyServersModel
        objectName: "proxyServersModel"

        sourceModel: ServersModel
        filters: [
            ValueFilter {
                roleName: "isCurrentlyProcessed"
                value: true
            }
        ]

        Component.onCompleted: {
            root.processedServer = proxyServersModel.get(0)
        }
    }

    Component.onCompleted: {
        PageController.showBusyIndicator(true)
        ApiConfigsController.prepareVpnKeyExport()
        PageController.showBusyIndicator(false)
    }

    FlickableType {
        anchors.fill: parent
        contentHeight: layout.implicitHeight

        ColumnLayout {
            id: layout
            width: root.width

            BackButtonType {
                Layout.topMargin: 20
            }

            Label {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 16
                text: qsTr(root.processedServer.name + "\nsubscription key")
                font.pixelSize: 32
                font.bold: true
                color: AmneziaStyle.color.paleGray
                wrapMode: Text.Wrap
            }

            BasicButtonType {
                Layout.fillWidth: true
                Layout.topMargin: 32
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Copy key")
                leftImageSource: "qrc:/images/controls/copy.svg"

                clickedFunc: function() {
                    ApiConfigsController.copyVpnKeyToClipboard()
                    PageController.showNotificationMessage(qsTr("Copied"))
                }
            }

            BasicButtonType {
                Layout.fillWidth: true
                Layout.topMargin: 4
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                defaultColor: "transparent"
                hoveredColor: AmneziaStyle.color.translucentWhite
                pressedColor: AmneziaStyle.color.sheerWhite
                textColor: AmneziaStyle.color.paleGray
                borderWidth: 1

                text: qsTr("Save key as a file")
                leftImageSource: "qrc:/images/controls/share-2.svg"

                clickedFunc: function() {
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
                        ExportController.exportConfig(fileName)
                        PageController.showBusyIndicator(false)
                    }
                }
            }

            BasicButtonType {
                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                defaultColor: "transparent"
                hoveredColor: AmneziaStyle.color.translucentWhite
                pressedColor: AmneziaStyle.color.sheerWhite
                textColor: AmneziaStyle.color.paleGray
                borderWidth: 1

                text: qsTr("Show key text")
                leftImageSource: "qrc:/images/controls/eye.svg"

                clickedFunc: function() {
                    PageController.showBusyIndicator(true)
                    ApiConfigsController.prepareVpnKeyExport()
                    PageController.showBusyIndicator(false)
                    vpnKeyDrawer.openTriggered()
                }
            }

            Rectangle {
                Layout.preferredWidth: Math.min(Math.min(root.width - (Layout.leftMargin + Layout.rightMargin), root.height * 0.5), 360)
                Layout.preferredHeight: Layout.preferredWidth
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 20
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                visible: ApiConfigsController.qrCodesCount > 0
                color: "white"
                radius: 12

                Image {
                    anchors.fill: parent
                    smooth: false
                    fillMode: Image.PreserveAspectFit
                    sourceSize.width: parent.width
                    sourceSize.height: parent.height
                    source: ApiConfigsController.qrCodesCount > 0 && ApiConfigsController.qrCodes[0] ? ApiConfigsController.qrCodes[0] : ""
                }
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.bottomMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                visible: ApiConfigsController.qrCodesCount > 0
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("To read the QR code in the Amnezia app, tap + in the main menu → 'QR code'")
            }
        }
    }

    DrawerType2 {
        id: vpnKeyDrawer

        anchors.fill: root
        expandedHeight: root.height * 0.9

        expandedStateContent: Item {
            BackButtonType {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.topMargin: 16
                backButtonFunction: function() { vpnKeyDrawer.closeTriggered() }
            }

            ColumnLayout {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.topMargin: 56
                anchors.leftMargin: 16
                anchors.rightMargin: 16

                Header2Type {
                    Layout.fillWidth: true
                    headerText: qsTr(root.processedServer.name + " Subscription key")
                }

                TextArea {
                    Layout.fillWidth: true
                    Layout.topMargin: 16
                    readOnly: true
                    color: AmneziaStyle.color.paleGray
                    selectionColor: AmneziaStyle.color.richBrown
                    selectedTextColor: AmneziaStyle.color.paleGray
                    font.pixelSize: 16
                    font.weight: Font.Medium
                    font.family: "PT Root UI VF"
                    text: ApiConfigsController.vpnKey
                    wrapMode: Text.Wrap
                    background: Rectangle { color: AmneziaStyle.color.transparent }
                }
            }
        }
    }
}
