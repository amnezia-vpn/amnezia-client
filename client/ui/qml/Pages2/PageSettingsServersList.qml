import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ProtocolEnum 1.0
import ContainerProps 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    ColumnLayout {
        id: header

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        anchors.topMargin: 20
        anchors.leftMargin: 16
        anchors.rightMargin: 16

        BackButtonType {
        }

        HeaderType {
            Layout.fillWidth: true

            actionButtonImage: "qrc:/images/controls/plus.svg"

            headerText: "Серверы"

            actionButtonFunction: function() {
                connectionTypeSelection.visible = true
            }
        }
    }

    ConnectionTypeSelectionDrawer {
        id: connectionTypeSelection
    }

    FlickableType {
        anchors.top: header.bottom
        anchors.topMargin: 16
        contentHeight: col.implicitHeight

        Column {
            id: col
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            ListView {
                id: servers
                width: parent.width
                height: servers.contentItem.height

                model: ServersModel

                clip: true
                interactive: false

                delegate: Item {
                    implicitWidth: servers.width
                    implicitHeight: delegateContent.implicitHeight

                    ColumnLayout {
                        id: delegateContent

                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right

                        LabelWithButtonType {
                            id: server
                            Layout.fillWidth: true

                            text: name
                            descriptionText: hostName
                            rightImageSource: "qrc:/images/controls/chevron-right.svg"

                            clickedFunction: function() {
                                ServersModel.setCurrentlyProcessedServerIndex(index)
                                ContainersModel.setCurrentlyProcessedServerIndex(index)
                                goToPage(PageEnum.PageSettingsServerInfo)
                            }
                        }

                        DividerType {}
                    }
                }
            }
        }
    }
}
