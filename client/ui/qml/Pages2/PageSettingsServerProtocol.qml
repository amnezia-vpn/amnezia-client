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
import "../Components/Protocols"

PageType {
    id: root

    FlickableType {
        id: fl
        anchors.fill: parent
        contentHeight: content.height + openVpnSettings.implicitHeight

        Column {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 16

            ListView {
                // todo change id naming
                id: container
                width: parent.width
                height: container.contentItem.height
                clip: true
                interactive: false
                model: SortFilterProxyModel {
                    id: proxyContainersModel
                    sourceModel: ContainersModel
                    filters: [
                        ValueFilter {
                            roleName: "isCurrentlyProcessed"
                            value: true
                        }
                    ]
                }

                delegate: Item {
                    implicitWidth: container.width
                    implicitHeight: delegateContent.implicitHeight

                    ColumnLayout {
                        id: delegateContent

                        anchors.fill: parent
                        anchors.rightMargin: 16
                        anchors.leftMargin: 16

                        HeaderType {
                            Layout.fillWidth: true
                            Layout.topMargin: 20

                            headerText: name
                        }
                    }
                }
            }

            OpenVpnSettings {
                id: openVpnSettings

                width: parent.width
            }
        }
    }
}
