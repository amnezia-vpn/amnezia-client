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

    Component.onCompleted: {
        var serverName = ServersModel.getProcessedServerData("name") || ServersModel.getProcessedServerData("hostName") || "Server"
        headerText = qsTr("Connection to ") + serverName
        configContentHeaderText = qsTr("File with connection settings to ") + serverName
    }
    property string configContentHeaderText
    property string shareButtonText: qsTr("Share")
    property string copyButtonText: qsTr("Copy")
    property bool isSelfHostedConfig: true

    property string configExtension: ".vpn"
    property string configCaption: qsTr("Save AmneziaVPN config")
    property string configFileName: "amnezia_config"

    onVisibleChanged: {
        configExtension = ".vpn"
        configCaption = qsTr("Save AmneziaVPN config")
        configFileName = "amnezia_config"
        
        if (visible) {
            var serverName = ServersModel.getProcessedServerData("name") || ServersModel.getProcessedServerData("hostName") || "Server"
            headerText = qsTr("Connection to ") + serverName
            configContentHeaderText = qsTr("File with connection settings to ") + serverName
        }
    }

    BackButtonType {
        id: backButton
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20
    }

    Text {
        id: shareHeader
        anchors.top: backButton.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20
        anchors.leftMargin: 16
        anchors.rightMargin: 16

        text: pageShareConnection.headerText
        color: AmneziaStyle.color.paleGray
        font.pixelSize: 32
        font.weight: 700
        font.family: "PT Root UI VF"
        wrapMode: Text.WordWrap
    }

    ListView {
        id: listView

        anchors.top: shareHeader.bottom
        anchors.topMargin: 16
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        property bool isFocusable: true

        ScrollBar.vertical: ScrollBarType {}
        model: 1
        clip: true
        reuseItems: true

        header: ColumnLayout {
            width: listView.width

            BasicButtonType {
                id: shareButton
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                text: pageShareConnection.shareButtonText
                leftImageSource: "qrc:/images/controls/share-2.svg"
                clickedFunc: function() {
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

            BasicButtonType {
                id: copyConfigTextButton
                Layout.fillWidth: true
                Layout.topMargin: 8
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                defaultColor: AmneziaStyle.color.transparent
                hoveredColor: AmneziaStyle.color.translucentWhite
                pressedColor: AmneziaStyle.color.sheerWhite
                disabledColor: AmneziaStyle.color.mutedGray
                textColor: AmneziaStyle.color.paleGray
                borderWidth: 1

                text: pageShareConnection.copyButtonText
                leftImageSource: "qrc:/images/controls/copy.svg"

                Keys.onReturnPressed: copyConfigTextButton.clicked()
                Keys.onEnterPressed: copyConfigTextButton.clicked()
            }

            BasicButtonType {
                id: copyNativeConfigStringButton
                Layout.fillWidth: true
                Layout.topMargin: 8
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                visible: false
                defaultColor: AmneziaStyle.color.transparent
                hoveredColor: AmneziaStyle.color.translucentWhite
                pressedColor: AmneziaStyle.color.sheerWhite
                disabledColor: AmneziaStyle.color.mutedGray
                textColor: AmneziaStyle.color.paleGray
                borderWidth: 1
                text: qsTr("Copy config string")
                leftImageSource: "qrc:/images/controls/copy.svg"
                KeyNavigation.tab: showSettingsButton
            }

            BasicButtonType {
                id: showSettingsButton
                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                visible: pageShareConnection.isSelfHostedConfig
                defaultColor: AmneziaStyle.color.transparent
                hoveredColor: AmneziaStyle.color.translucentWhite
                pressedColor: AmneziaStyle.color.sheerWhite
                disabledColor: AmneziaStyle.color.mutedGray
                textColor: AmneziaStyle.color.paleGray
                borderWidth: 1
                text: qsTr("Show connection settings")
                clickedFunc: function() {
                    configContentDrawer.openTriggered()
                }
            }

            DrawerType2 {
                id: configContentDrawer
                parent: pageShareConnection.parent
                anchors.fill: parent
                expandedHeight: parent.height * 0.9
                expandedStateContent: Item {
                    id: configContentContainer
                    implicitHeight: configContentDrawer.expandedHeight

                    Connections {
                        target: copyNativeConfigStringButton
                        function onClicked() {
                            nativeConfigString.selectAll()
                            nativeConfigString.copy()
                            nativeConfigString.select(0, 0)
                            PageController.showNotificationMessage(qsTr("Copied"))
                        }
                    }

                    Connections {
                        target: copyConfigTextButton
                        function onClicked() {
                            configText.selectAll()
                            configText.copy()
                            configText.select(0, 0)
                            PageController.showNotificationMessage(qsTr("Copied"))
                            header.forceActiveFocus()
                        }
                    }

                    BackButtonType {
                        id: configBackButton
                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.topMargin: 16
                        backButtonFunction: function() { configContentDrawer.closeTriggered() }
                    }

                    FlickableType {
                        anchors.top: configBackButton.bottom
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        contentHeight: configContent.implicitHeight + configContent.anchors.topMargin + configContent.anchors.bottomMargin

                        ColumnLayout {
                            id: configContent
                            anchors.fill: parent
                            anchors.rightMargin: 16
                            anchors.leftMargin: 16

                            Header2Type {
                                id: configContentHeader
                                Layout.fillWidth: true
                                Layout.topMargin: 16
                                headerText: pageShareConnection.configContentHeaderText
                            }

                            TextField {
                                id: nativeConfigString
                                visible: false
                                text: ExportController.nativeConfigString
                                onTextChanged: copyNativeConfigStringButton.visible = nativeConfigString.text !== ""
                            }

                            TextArea {
                                id: configText
                                Layout.fillWidth: true
                                Layout.topMargin: 16
                                Layout.bottomMargin: 16
                                padding: 0
                                leftPadding: 0
                                height: 24
                                readOnly: true
                                activeFocusOnTab: false
                                color: AmneziaStyle.color.paleGray
                                selectionColor:  AmneziaStyle.color.richBrown
                                selectedTextColor: AmneziaStyle.color.paleGray
                                font.pixelSize: 16
                                font.weight: Font.Medium
                                font.family: "PT Root UI VF"
                                text: ExportController.config
                                wrapMode: Text.Wrap
                                background: Rectangle { color: AmneziaStyle.color.transparent }
                            }
                        }
                    }
                }
            }
        }

        delegate: ColumnLayout {
            width: listView.width
            property bool isQrCodeVisible: pageShareConnection.isSelfHostedConfig ? ExportController.qrCodesCount > 0 : ApiConfigsController.qrCodesCount > 0

            Rectangle {
                id: qrCodeContainer
                Layout.fillWidth: true
                Layout.preferredHeight: width
                Layout.topMargin: 20
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                visible: isQrCodeVisible
                color: "white"

                Image {
                    anchors.fill: parent
                    smooth: false
                    source: pageShareConnection.isSelfHostedConfig ? (isQrCodeVisible ? ExportController.qrCodes[0] : "") : (isQrCodeVisible ? ApiConfigsController.qrCodes[0] : "")
                    property bool isFocusable: true
                    Keys.onTabPressed: FocusController.nextKeyTabItem()
                    Keys.onBacktabPressed: FocusController.previousKeyTabItem()
                    Keys.onUpPressed: FocusController.nextKeyUpItem()
                    Keys.onDownPressed: FocusController.nextKeyDownItem()
                    Keys.onLeftPressed: FocusController.nextKeyLeftItem()
                    Keys.onRightPressed: FocusController.nextKeyRightItem()

                    Timer {
                        property int index: 0
                        interval: 1000
                        running: isQrCodeVisible
                        repeat: true
                        onTriggered: {
                            if (isQrCodeVisible) {
                                index++
                                let qrCodesCount = pageShareConnection.isSelfHostedConfig ? ExportController.qrCodesCount : ApiConfigsController.qrCodesCount
                                if (index >= qrCodesCount) index = 0
                                parent.source = pageShareConnection.isSelfHostedConfig ? ExportController.qrCodes[index] : ApiConfigsController.qrCodes[index]
                            }
                        }
                    }

                    Behavior on source { PropertyAnimation { duration: 200 } }
                }
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.bottomMargin: 32
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                visible: isQrCodeVisible
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("To read the QR code in the Amnezia app, select \"Add server\" → \"I have data to connect\" → \"QR code, key or settings file\"")
            }
        }
    }
}
