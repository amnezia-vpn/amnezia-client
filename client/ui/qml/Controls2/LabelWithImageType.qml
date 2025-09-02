import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "TextTypes"

RowLayout {
    id: root

    property string imageSource
    property string leftText
    property var rightText
    property bool isRightTextUndefined: rightText === undefined
    property int rightTextFormat: Text.PlainText

    visible: !isRightTextUndefined

    Image {
        Layout.preferredHeight: 18
        Layout.preferredWidth: 18
        source: root.imageSource
    }

    ListItemTitleType {
        Layout.fillWidth: true
        Layout.rightMargin: 10
        Layout.alignment: Qt.AlignRight

        text: root.leftText
    }

    ParagraphTextType {
        visible: root.rightText !== ""

        Layout.alignment: Qt.AlignLeft

        text: root.isRightTextUndefined ? "" : root.rightText
        textFormat: root.rightTextFormat
    }
}
