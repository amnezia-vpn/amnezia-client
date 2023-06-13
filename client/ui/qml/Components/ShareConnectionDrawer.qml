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

    property var qrCodes: []
    property alias configText: configContent.text
    property alias headerText: header.headerText

    width: parent.width
    height: parent.height * 0.9

    Item{
        anchors.fill: parent

        FlickableType {
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            contentHeight: content.height + 32

            ColumnLayout {
                id: content

                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right

                anchors.topMargin: 20
                anchors.leftMargin: 16
                anchors.rightMargin: 16

                Header2Type {
                    id: header
                    Layout.fillWidth: true
                }

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

                    text: showContent ? qsTr("Collapse content") : qsTr("Show content")

                    onClicked: {
                        showContent = !showContent
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: configContent.implicitHeight + configContent.anchors.topMargin + configContent.anchors.bottomMargin

                    radius: 10
                    color: "#2C2D30"

                    visible: showContent

                    height: 24

                    TextField {
                        id: configContent

                        anchors.fill: parent
                        anchors.margins: 16

                        height: 24

                        color: "#D7D8DB"

                        font.pixelSize: 16
                        font.weight: Font.Medium
                        font.family: "PT Root UI VF"

                        wrapMode: Text.Wrap

                        enabled: false
                        background: Rectangle {
                            anchors.fill: parent
                            color: "transparent"
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: width
                    Layout.topMargin: 20

                    color: "white"

                    Image {
                        anchors.fill: parent
                        smooth: false

                        Timer {
                            property int idx: 0
                            interval: 1000
                            running: qrCodes.length > 0
                            repeat: true
                            onTriggered: {
                                idx++
                                if (idx >= qrCodes.length) {
                                    idx = 0
                                }
                                parent.source = qrCodes[idx]
                            }
                        }

                        Behavior on source {
                            PropertyAnimation { duration: 200 }
                        }

                        visible: qrCodes.length > 0
                    }
                }

                ParagraphTextType {
                    Layout.fillWidth: true
                    Layout.topMargin: 24
                    Layout.bottomMargin: 32

                    horizontalAlignment: Text.AlignHCenter
                    text: qsTr("To read the QR code in the Amnezia app, select \"Add Server\" → \"I have connection details\"")
                }
            }
        }
    }
}
