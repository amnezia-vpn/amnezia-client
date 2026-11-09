import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Config"

PageType {
    id: root

    ListViewType {
        id: newsList

        anchors.fill: parent

        model: NewsModel

        header: ColumnLayout {
            width: newsList.width

            z: 2

            spacing: 0

            NavigationBarType {
                Layout.fillWidth: true

                title: qsTr("News & Notifications")
            }
        }

        delegate: Item {
            width: newsList.width
            implicitHeight: newsItem.implicitHeight

            ListItemType {
                id: newsItem

                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16

                icon.source: read ? "" : "qrc:/images/controls/unread-dot.svg"
                icon.width: 8
                icon.height: 8
                icon.color: AmneziaStyle.color.accentLight
                text: title
                subtitle: Qt.formatDateTime(timestamp, "dd.MM.yyyy HH:mm")
                trailingIconSource: "qrc:/images/controls/chevron-right.svg"

                onClicked: {
                    NewsModel.markAsRead(index)
                    NewsModel.processedIndex = index
                    PageController.goToPage(PageEnum.PageSettingsNewsDetail)
                }
            }
        }

        footer: Item {
            width: newsList.width
            height: 16 + PageController.safeAreaBottomMargin
        }
    }
}
