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

DrawerType2 {
    id: root

    property string headerText
    property string configContentHeaderText
    property string contentVisible

    property string configExtension: ".vpn"
    property string configCaption: qsTr("Save AmneziaVPN config")
    property string configFileName: "amnezia_config"

    expandedHeight: parent.height * 0.9

    onClosed: {
        configExtension = ".vpn"
        configCaption = qsTr("Save AmneziaVPN config")
        configFileName = "amnezia_config"
    }

    expandedStateContent: Item {
        implicitHeight: root.expandedHeight

        Connections {
            target: root
            enabled: !GC.isMobile()
            function onOpened() {
                header.forceActiveFocus()
            }
        }

        Header2Type {
            id: header
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 20
            anchors.leftMargin: 16
            anchors.rightMargin: 16

            headerText: root.headerText
        }

        FlickableType {
            anchors.top: header.bottom
            anchors.bottom: parent.bottom
            contentHeight: content.height + 32

            ColumnLayout {
                id: content

                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right

                anchors.leftMargin: 16
                anchors.rightMargin: 16

                visible: root.contentVisible

                BasicButtonType {
                    id: shareButton
                    Layout.fillWidth: true
                    Layout.topMargin: 16

                    text: qsTr("Share")
                    imageSource: "qrc:/images/controls/share-2.svg"

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

                    defaultColor: AmneziaStyle.color.transparent
                    hoveredColor: AmneziaStyle.color.translucentWhite
                    pressedColor: AmneziaStyle.color.sheerWhite
                    disabledColor: AmneziaStyle.color.mutedGray
                    textColor: AmneziaStyle.color.paleGray
                    borderWidth: 1

                    text: qsTr("Copy")
                    imageSource: "qrc:/images/controls/copy.svg"

                    Keys.onReturnPressed: { copyConfigTextButton.clicked() }
                    Keys.onEnterPressed: { copyConfigTextButton.clicked() }
                }

                BasicButtonType {
                    id: copyNativeConfigStringButton
                    Layout.fillWidth: true
                    Layout.topMargin: 8

                    visible: false

                    defaultColor: AmneziaStyle.color.transparent
                    hoveredColor: AmneziaStyle.color.translucentWhite
                    pressedColor: AmneziaStyle.color.sheerWhite
                    disabledColor: AmneziaStyle.color.mutedGray
                    textColor: AmneziaStyle.color.paleGray
                    borderWidth: 1

                    text: qsTr("Copy config string")
                    imageSource: "qrc:/images/controls/copy.svg"
                }

                BasicButtonType {
                    id: showSettingsButton

                    Layout.fillWidth: true
                    Layout.topMargin: 24

                    defaultColor: AmneziaStyle.color.transparent
                    hoveredColor: AmneziaStyle.color.translucentWhite
                    pressedColor: AmneziaStyle.color.sheerWhite
                    disabledColor: AmneziaStyle.color.mutedGray
                    textColor: AmneziaStyle.color.paleGray
                    borderWidth: 1

                    text: qsTr("Show connection settings")

                    clickedFunc: function() {
                        configContentDrawer.open()
                    }
                }

                DrawerType2 {
                    id: configContentDrawer

                    parent: root.parent

                    anchors.fill: parent
                    expandedHeight: parent.height * 0.9

                    onClosed: {
                        if (!GC.isMobile()) {
                            header.forceActiveFocus()
                        }
                    }

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
                            id: backButton

                            anchors.top: parent.top
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.topMargin: 16

                            backButtonFunction: function() { configContentDrawer.close() }
                        }

                        FlickableType {
                            anchors.top: backButton.bottom
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

                                    headerText: root.configContentHeaderText
                                }

                                TextField {
                                    id: nativeConfigString
                                    visible: false
                                    text: ExportController.nativeConfigString

                                    onTextChanged: {
                                        copyNativeConfigStringButton.visible = nativeConfigString.text !== ""
                                    }
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

                                    background: Rectangle {
                                        color: AmneziaStyle.color.transparent
                                    }
                                }
                            }
                        }
                    }
                }

                Rectangle {
                    id: qrCodeContainer

                    Layout.fillWidth: true
                    Layout.preferredHeight: width
                    Layout.topMargin: 20

                    visible: ExportController.qrCodesCount > 0

                    color: "white"

                    Image {
                        anchors.fill: parent
                        smooth: false

                        source: ExportController.qrCodesCount ? ExportController.qrCodes[0] : ""

                        Timer {
                            property int index: 0
                            interval: 1000
                            running: ExportController.qrCodesCount > 0
                            repeat: true
                            onTriggered: {
                                if (ExportController.qrCodesCount > 0) {
                                    index++
                                    if (index >= ExportController.qrCodesCount) {
                                        index = 0
                                    }
                                    parent.source = ExportController.qrCodes[index]
                                }
                            }
                        }

                        Behavior on source {
                            PropertyAnimation { duration: 200 }
                        }
                    }
                }

                ParagraphTextType {
                    Layout.fillWidth: true
                    Layout.topMargin: 24
                    Layout.bottomMargin: 32

                    visible: ExportController.qrCodesCount > 0

                    horizontalAlignment: Text.AlignHCenter
                    text: qsTr("To read the QR code in the Amnezia app, select \"Add server\" → \"I have data to connect\" → \"QR code, key or settings file\"")
                }
            }
        }
    }
}
