import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ProtocolEnum 1.0
import ContainerProps 1.0
import ProtocolProps 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    SortFilterProxyModel {
        id: proxyServersModel
        sourceModel: ServersModel
        filters: [
            ValueFilter {
                roleName: "isCurrentlyProcessed"
                value: true
            }
        ]
    }

    ColumnLayout {
        anchors.fill: parent

        spacing: 16

        Repeater {
            id: header
            model: proxyServersModel

            delegate: ColumnLayout {
                id: content

                Layout.topMargin: 20
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                BackButtonType {
                }

                HeaderType {
                    Layout.fillWidth: true

                    actionButtonImage: "qrc:/images/controls/edit-3.svg"

                    headerText: name
                    descriptionText: hostName

                    actionButtonFunction: function() {
                        connectionTypeSelection.visible = true
                    }
                }
            }
        }

        TabBar {
            id: tabBar

            Layout.fillWidth: true

            background: Rectangle {
                color: "transparent"
            }

            TabButtonType {
                isSelected: tabBar.currentIndex === 0
                text: qsTr("Protocols")
            }
            TabButtonType {
                isSelected: tabBar.currentIndex === 1
                text: qsTr("Services")
            }
            TabButtonType {
                isSelected: tabBar.currentIndex === 2
                text: qsTr("Data")
            }
        }

        StackLayout {
            Layout.preferredWidth: root.width
            Layout.preferredHeight: root.height - tabBar.implicitHeight - header.implicitHeight

            currentIndex: tabBar.currentIndex

            PageSettingsServerProtocols {
                stackView: root.stackView
            }
            PageSettingsServerServices {
                stackView: root.stackView
            }
            PageSettingsServerData {
                stackView: root.stackView
            }
        }
    }
}
