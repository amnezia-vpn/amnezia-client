import QtQuick
import QtQuick.Layouts

import Style 1.0

import "TextTypes"

Item {
    id: root

    property string headerText
    property int headerTextMaximumLineCount: 2
    property int headerTextElide: Qt.ElideRight
    property string descriptionText
    property string descriptionLinkText
    property string descriptionLinkUrl
    property alias headerRow: headerRow

    implicitWidth: content.implicitWidth
    implicitHeight: content.implicitHeight

    ColumnLayout {
        id: content
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        RowLayout {
            id: headerRow
            
            Header1TextType {
                id: header
                Layout.fillWidth: true
                text: root.headerText
                maximumLineCount: root.headerTextMaximumLineCount
                elide: root.headerTextElide
            }
        }

        ParagraphTextType {
            id: description
            Layout.topMargin: 16
            Layout.fillWidth: true
            text: root.descriptionText
            color: AmneziaStyle.color.mutedGray
            visible: root.descriptionText !== ""
        }

        ParagraphTextType {
            id: descriptionLink
            Layout.topMargin: 16
            Layout.fillWidth: true
            text: root.descriptionLinkText !== "" && root.descriptionLinkUrl !== ""
                  ? ("<a href=\"" + root.descriptionLinkUrl + "\" style=\"color: " + AmneziaStyle.color.goldenApricotString + ";\">" + root.descriptionLinkText + "</a>")
                  : ""
            textFormat: Text.RichText
            visible: root.descriptionLinkText !== ""

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
