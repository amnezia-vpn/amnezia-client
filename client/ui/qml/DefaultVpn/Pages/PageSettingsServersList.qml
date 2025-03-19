pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Config 1.0

import "../Components"
import "../Controls"
import "../Controls/TextTypes"

Page {
    id: root

    ColumnLayout {
        anchors.fill: parent

        RowLayout {
            Layout.leftMargin: 8
            Layout.rightMargin: 8
            Layout.topMargin: 8

            WhiteButtonNoBorder {
                id: backButton
                imageSource: "qrc:/images/controls/arrow-left.svg"

                onClicked: PageController.closePage()
            }

            Item {
                Layout.fillWidth: true
            }

            WhiteButtonNoBorder {
                imageSource: "qrc:/images/controls/plus.svg"

                onClicked: function() {
                    PageController.goToPage(PageEnum.PageSetupWizardConfigSource)
                }
            }
        }

        Header1TextType {
            id: header

            Layout.topMargin: 8
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.fillWidth: true

            text: qsTr("Connect to")

            horizontalAlignment: Qt.AlignLeft
            verticalAlignment: Qt.AlignVCenter
        }

        ButtonGroup {
            id: serversRadioButtonGroup
        }

        ListView {
            id: serversListView

            Layout.topMargin: 16
            Layout.fillHeight: true
            Layout.fillWidth: true

            model: ServersModel
            currentIndex: ServersModel.defaultIndex

            ScrollBar.vertical: ScrollBar {}

            Connections {
                target: ServersModel
                function onDefaultServerIndexChanged(serverIndex) {
                    serversListView.currentIndex = serverIndex
                    serversListView.positionViewAtIndex(serversListView.currentIndex, ListView.Contain)
                }
            }

            Component.onCompleted: positionViewAtIndex(currentIndex, ListView.Center)

            delegate: Item {
                id: menuContentDelegate
                required property string name
                required property int index

                implicitWidth: serversListView.width
                implicitHeight: serverItem.implicitHeight

                RadioButton {
                    id: serverItem

                    anchors.fill: parent
                    anchors.rightMargin: 16
                    anchors.leftMargin: 16

                    ButtonGroup.group: serversRadioButtonGroup

                    checked: index === serversListView.currentIndex

                    indicator: Item { }

                    contentItem: Item {
                        id: contentContainer

                        anchors.left: parent.left
                        anchors.right: parent.right

                        implicitHeight: content.implicitHeight

                        Rectangle {
                            anchors.fill: parent

                            radius: 8

                            color: serverItem.checked ? Style.color.gray1 : Style.color.transparent
                        }

                        RowLayout {
                            id: content
                            anchors.fill: parent

                            Header3TextType {
                                Layout.fillWidth: true
                                Layout.leftMargin: 8
                                Layout.topMargin: 19
                                Layout.bottomMargin: 19

                                text: name

                                color: serverItem.hovered ? Style.color.gray9 : Style.color.black
                            }

                            ButtonType {
                                Layout.rightMargin: 8
                                imageSource: "qrc:/images/controls/edit-3.svg"

                                hoveredBorderColor: Style.color.gray2
                                hoveredBorderWidth: 1

                                onClicked: function() {
                                    ServersModel.processedIndex = index
                                    PageController.goToPage(PageEnum.PageSettingsServerInfo)
                                }
                            }
                        }
                    }

                    onClicked: function() {
                        ServersModel.defaultIndex = index
                    }

                    MouseArea {
                        anchors.fill: serverItem
                        cursorShape: Qt.PointingHandCursor
                        enabled: false
                    }
                }
            }
        }
    }
}
