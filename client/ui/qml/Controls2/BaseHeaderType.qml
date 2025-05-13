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
    property alias headerRow: headerRow

    implicitWidth: content.implicitWidth
    implicitHeight: content.implicitHeight

    ColumnLayout {
        id: content
        anchors.fill: parent

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
    }
} 
