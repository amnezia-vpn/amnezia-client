import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ContainerProps 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"

DrawerType {
    id: root

    property alias headerText: header.headerText
    property alias configContentHeaderText: configContentHeader.headerText
    property alias contentVisible: content.visible

    width: parent.width
    height: parent.height * 0.9

    Item {
        anchors.fill: parent

        Header2Type {
            id: header
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 20
            anchors.leftMargin: 16
            anchors.rightMargin: 16
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

                BasicButtonType {
                    Layout.fillWidth: true
                    Layout.topMargin: 16

                    text: qsTr("Save connection code")

                    onClicked: {
                        ExportController.saveFile()
                    }
                }

                BasicButtonType {
                    Layout.fillWidth: true
                    Layout.topMargin: 8

                    defaultColor: "transparent"
                    hoveredColor: Qt.rgba(1, 1, 1, 0.08)
                    pressedColor: Qt.rgba(1, 1, 1, 0.12)
                    disabledColor: "#878B91"
                    textColor: "#D7D8DB"
                    borderWidth: 1

                    text: qsTr("Copy")

                    onClicked: {
                        configContent.selectAll()
                        configContent.copy()
                        configContent.select(0, 0)
                    }
                }

                BasicButtonType {
                    Layout.fillWidth: true
                    Layout.topMargin: 8

                    defaultColor: "transparent"
                    hoveredColor: Qt.rgba(1, 1, 1, 0.08)
                    pressedColor: Qt.rgba(1, 1, 1, 0.12)
                    disabledColor: "#878B91"
                    textColor: "#D7D8DB"

                    text: qsTr("Show content")

                    onClicked: {
                        configContentDrawer.visible = true
                    }
                }

                DrawerType {
                    id: configContentDrawer

                    width: parent.width
                    height: parent.height * 0.9

                    BackButtonType {
                        id: backButton

                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.topMargin: 16

                        backButtonFunction: function() {
                            configContentDrawer.visible = false
                        }
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
                            }

                            TextArea {
                                Layout.fillWidth: true
                                Layout.topMargin: 16
                                Layout.bottomMargin: 16

                                padding: 0
                                leftPadding: 0
                                height: 24

                                color: "#D7D8DB"
                                selectionColor:  "#412102"
                                selectedTextColor: "#D7D8DB"

                                font.pixelSize: 16
                                font.weight: Font.Medium
                                font.family: "PT Root UI VF"

                                text: ExportController.formattedConfig

                                wrapMode: Text.Wrap

                                background: Rectangle {
                                    color: "transparent"
                                }
                            }
                        }
                    }
                }

                Rectangle {
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
                    text: qsTr("To read the QR code in the Amnezia app, select \"Add Server\" → \"I have connection details\"")
                }
            }
        }
    }
}
