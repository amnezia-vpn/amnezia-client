import QtQuick
import QtQuick.Layouts

import Style 1.0

import "TextTypes"

ColumnLayout {
    id: root

    property string overline
    property string title
    property string subtitle
    property color titleColor: AmneziaStyle.color.textPrimary
    property color subtitleColor: AmneziaStyle.color.textTertiary
    property int maximumLineCount: 2
    property int subtitleLineHeight: 20
    property real textOpacity: 1.0
    property bool dense: false

    spacing: 0

    SmallTextType {
        Layout.fillWidth: true

        visible: !root.dense && text !== ""
        text: root.overline
        opacity: root.textOpacity
    }

    CaptionTextType {
        Layout.fillWidth: true

        visible: root.dense && text !== ""
        text: root.overline
        opacity: root.textOpacity
    }

    SmallTextType {
        Layout.fillWidth: true

        visible: root.dense && text !== ""
        text: root.title
        color: AmneziaStyle.color.textSecondary
        opacity: root.textOpacity
        maximumLineCount: root.maximumLineCount
        elide: Text.ElideRight
    }

    H3TextType {
        Layout.fillWidth: true

        visible: !root.dense && text !== ""
        text: root.title
        color: root.titleColor
        opacity: root.textOpacity
        maximumLineCount: root.maximumLineCount
        elide: Text.ElideRight

        Behavior on color {
            ColorAnimation { duration: 150 }
        }
    }

    SmallTextType {
        Layout.fillWidth: true

        visible: text !== ""
        text: root.subtitle
        color: root.subtitleColor
        opacity: root.textOpacity
        lineHeight: root.subtitleLineHeight + LanguageUiController.getLineHeightAppend()
    }
}
