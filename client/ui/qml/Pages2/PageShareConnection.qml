import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import QtCore

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ContainerProps 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: pageShareConnection

    property string headerText
    property string configContentHeaderText
    property string shareButtonText: qsTr("Share")
    property string copyButtonText: qsTr("Copy")
    property bool isSelfHostedConfig: true

    property string configExtension: ".vpn"
    property string configCaption: qsTr("Save AmneziaVPN config")
    property string configFileName: "amnezia_config"

    readonly property bool isQrCodeVisible: pageShareConnection.isSelfHostedConfig ? ExportController.qrCodesCount > 0
                                                                                   : SubscriptionUiController.qrCodesCount > 0

    function copyText(field) {
        field.selectAll()
        field.copy()
        field.select(0, 0)
        PageController.showNotificationMessage(qsTr("Copied"))
    }

    TextField {
        id: nativeConfigString

        visible: false
        text: ExportController.nativeConfigString
    }

    FlickableType {
        id: flickable

        anchors.fill: parent

        contentHeight: content.implicitHeight

        ColumnLayout {
            id: content

            width: flickable.width

            spacing: 0

            NavigationBarType {
                Layout.fillWidth: true

                title: pageShareConnection.headerText
            }

            ButtonType {
                id: shareButton

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: pageShareConnection.shareButtonText
                leftImageSource: "qrc:/images/controls/share-2.svg"

                onClicked: {
                    var fileName = ""
                    if (GC.isMobile()) {
                        fileName = configFileName + configExtension
                    } else {
                        fileName = SystemController.getFileName(configCaption,
                                                                qsTr("Config files (*" + configExtension + ")"),
                                                                StandardPaths.standardLocations(StandardPaths.DocumentsLocation) + "/" + configFileName,
                                                                true,
                                                                configExtension)
                    }
                    if (fileName !== "") {
                        PageController.showBusyIndicator(true)
                        ExportController.exportConfig(fileName)
                        PageController.showBusyIndicator(false)
                    }
                }
            }

            ButtonType {
                id: copyConfigTextButton

                Layout.fillWidth: true
                Layout.topMargin: 12
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                variant: ButtonType.Secondary
                text: pageShareConnection.copyButtonText
                leftImageSource: "qrc:/images/controls/copy.svg"

                onClicked: pageShareConnection.copyText(configContentDrawer.textArea)
            }

            ButtonType {
                id: copyNativeConfigStringButton

                Layout.fillWidth: true
                Layout.topMargin: 12
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                visible: nativeConfigString.text !== ""

                variant: ButtonType.Secondary
                text: qsTr("Copy config string")
                leftImageSource: "qrc:/images/controls/copy.svg"

                onClicked: pageShareConnection.copyText(nativeConfigString)
            }

            ButtonType {
                id: showSettingsButton

                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                visible: pageShareConnection.isSelfHostedConfig

                variant: ButtonType.Secondary
                text: qsTr("Show connection settings")

                onClicked: configContentDrawer.open()
            }

            QrCodeCard {
                id: qrCodeCard

                property int index: 0

                Layout.fillWidth: true
                Layout.maximumWidth: 360
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                visible: pageShareConnection.isQrCodeVisible

                source: pageShareConnection.isQrCodeVisible
                        ? (pageShareConnection.isSelfHostedConfig ? ExportController.qrCodes[0] : SubscriptionUiController.qrCodes[0])
                        : ""

                Timer {
                    interval: 1000
                    running: pageShareConnection.isQrCodeVisible
                    repeat: true

                    onTriggered: {
                        if (pageShareConnection.isQrCodeVisible) {
                            qrCodeCard.index++
                            let qrCodesCount = pageShareConnection.isSelfHostedConfig ? ExportController.qrCodesCount
                                                                                      : SubscriptionUiController.qrCodesCount
                            if (qrCodeCard.index >= qrCodesCount) {
                                qrCodeCard.index = 0
                            }
                            qrCodeCard.source = pageShareConnection.isSelfHostedConfig ? ExportController.qrCodes[qrCodeCard.index]
                                                                                       : SubscriptionUiController.qrCodes[qrCodeCard.index]
                        }
                    }
                }
            }

            SmallTextType {
                Layout.fillWidth: true
                Layout.topMargin: 12
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                visible: pageShareConnection.isQrCodeVisible
                         && !(pageShareConnection.isSelfHostedConfig
                              && (ExportController.config.startsWith("tg://")
                                  || ExportController.config.startsWith("https://t.me")))

                horizontalAlignment: Text.AlignHCenter
                color: AmneziaStyle.color.textTertiary
                text: qsTr("To read the QR code in the Amnezia app, tap + in the main menu → 'QR code'")
            }

            NotificationType {
                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                visible: pageShareConnection.isSelfHostedConfig
                         && pageShareConnection.configExtension === ".conf"
                         && !pageShareConnection.isQrCodeVisible
                         && ExportController.config !== ""

                variant: NotificationType.Warning
                text: qsTr("This config is too large for a QR code. Share the file or copy the connection settings instead.")
            }

            Item {
                Layout.preferredHeight: 24 + PageController.safeAreaBottomMargin
            }
        }
    }

    ConfigContentSheet {
        id: configContentDrawer

        headerText: pageShareConnection.configContentHeaderText
        contentText: ExportController.config
    }
}
