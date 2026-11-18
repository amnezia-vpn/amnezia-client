import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import SortFilterProxyModel 0.2

PageType {
    id: root
    property var newsItem

    SortFilterProxyModel {
        id: proxyNews
        sourceModel: NewsModel
        filters: [ ValueFilter { roleName: "isProcessed"; value: true } ]
        Component.onCompleted: root.newsItem = proxyNews.get(0)
    }

    Connections {
        target: NewsModel
        function onProcessedIndexChanged() {
            root.newsItem = proxyNews.get(0)
        }
    }

    FlickableType {
        id: fl

        anchors.top: parent.top

        contentHeight: content.implicitHeight

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 0

            NavigationBarType {
                Layout.fillWidth: true

                title: newsItem ? newsItem.title : ""
            }

            BodyTextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 16 + PageController.safeAreaBottomMargin

                text: newsItem ? newsItem.content : ""

                textFormat: Text.RichText

                onLinkActivated: function(link) {
                    Qt.openUrlExternally(link)
                }

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.NoButton
                    cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
                }
            }
        }
    }
}
