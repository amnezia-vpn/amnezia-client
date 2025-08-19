import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"

PageType {
    id: root

    Component.onCompleted: PageController.disableTabBar(true)
    Component.onDestruction: PageController.disableTabBar(false)

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

    ListViewType {
        id: listView

        anchors.fill: parent

        spacing: 16

        model: proxyServersModel

        delegate: ColumnLayout {
            width: listView.width

            BaseHeaderType {
                Layout.fillWidth: true
                Layout.topMargin: 20
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("Removing services from %1").arg(name)
            }

            ProgressBarType {
                id: progressBar

                Layout.fillWidth: true
                Layout.topMargin: 32
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                Timer {
                    id: timer

                    interval: 300
                    repeat: true
                    running: true
                    onTriggered: {
                        progressBar.value += 0.003
                    }
                }
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.topMargin: 8
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Usually it takes no more than 5 minutes")
            }
        }
    }
}
