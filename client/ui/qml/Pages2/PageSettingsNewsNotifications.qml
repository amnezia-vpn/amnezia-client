import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"

PageType {
    id: root

    ColumnLayout {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        anchors.topMargin: 20

        BackButtonType {
            id: backButton
        }

        BaseHeaderType {
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            
            headerText: qsTr("News & Notifications")
        }
    }

    ListView {
        id: newsList
        width: parent.width
        anchors.top: header.bottom
        anchors.topMargin: 16
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        property bool isFocusable: true

        model: NewsModel
        
        clip: true
        reuseItems: true

        delegate: Item {
            implicitWidth: newsList.width
            implicitHeight: content.implicitHeight

            ColumnLayout {
                id: content
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right

                LabelWithButtonType {
                    Layout.fillWidth: true
                    leftImageSource: read ? "" : "qrc:/images/controls/unread-dot.svg"
                    isSmallLeftImage: !read
                    text: title
                    rightImageSource: "qrc:/images/controls/chevron-right.svg"

                    clickedFunction: function() {
                            NewsModel.markAsRead(index)
                            NewsModel.processedIndex = index
                        PageController.goToPage(PageEnum.PageSettingsNewsDetail)
                    }
                }

                DividerType {}
            }
        }
    }
} 
