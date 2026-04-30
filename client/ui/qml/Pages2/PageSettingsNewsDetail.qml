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
    property bool isUpdateItem: newsItem && (newsItem.isUpdate !== undefined ? newsItem.isUpdate : false)

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
        anchors.topMargin: 20 + PageController.safeAreaTopMargin
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
                headerText: newsItem ? newsItem.title : ""
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                text: newsItem ? newsItem.content : ""

                textFormat: root.isUpdateItem ? Text.MarkdownText : Text.RichText

                onLinkActivated: function(link) {
                    Qt.openUrlExternally(link)
                }

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.NoButton
                    cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
                }
            }

            BasicButtonType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 24
                visible: root.isUpdateItem
                text: qsTr("Update")

                clickedFunc: function() {
                    if (!root.isUpdateItem)
                        return
                    PageController.showBusyIndicator(true)
                    UpdateController.runInstaller()
                    PageController.showBusyIndicator(false)
                    PageController.closePage()
                }
            }

            BasicButtonType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 8
                Layout.bottomMargin: 16
                visible: root.isUpdateItem
                defaultColor: "transparent"
                hoveredColor: Qt.rgba(1, 1, 1, 0.08)
                pressedColor: Qt.rgba(1, 1, 1, 0.12)
                disabledColor: "#878B91"
                textColor: "#D7D8DB"
                borderWidth: 1
                text: qsTr("Skip")

                clickedFunc: function() {
                    if (!root.isUpdateItem)
                        return
                    NewsModel.markUpdateAsSkipped()
                    PageController.closePage()
                }
            }
        }
    }
} 
