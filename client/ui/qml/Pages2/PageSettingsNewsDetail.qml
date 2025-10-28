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

    BackButtonType {
        id: backButton
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20
    }

    FlickableType {
        id: fl
        anchors.top: backButton.bottom
        anchors.bottom: parent.bottom
        contentHeight: content.height

        ColumnLayout {
            id: content
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            spacing: 0

            BaseHeaderType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                headerText: newsItem.title
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                text: newsItem.content
            }
        }
    }
} 
