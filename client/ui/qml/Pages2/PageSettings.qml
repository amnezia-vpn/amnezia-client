import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ContainerProps 1.0
import ProtocolProps 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"

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

    FlickableType {
        id: fl
        anchors.fill: parent
        contentHeight: content.height

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 16

            Repeater {
                model: proxyServersModel

                delegate: HeaderType {
                    id: header

                    Layout.fillWidth: true
                    Layout.topMargin: 20
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16

                    actionButtonImage: "qrc:/images/controls/plus.svg"
                    backButtonImage: "qrc:/images/controls/arrow-left.svg"

                    headerText: name

                    actionButtonFunction: function() {
                        connectionTypeSelection.visible = true
                    }

                    backButtonFunction: function() {
                        closePage()
                    }
                }
            }
        }
    }
}
