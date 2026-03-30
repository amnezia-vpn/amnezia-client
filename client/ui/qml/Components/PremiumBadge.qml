import QtQuick
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

import Style 1.0
import "../Controls2/TextTypes"

Rectangle {
    id: root

    property string text: ""
    property string tone: "neutral"
    property string iconSource: ""
    property bool compact: false

    readonly property color backgroundColor: {
        switch (tone) {
        case "accent": return Qt.rgba(0, 200/255, 255/255, 0.16)
        case "success": return Qt.rgba(16/255, 185/255, 129/255, 0.18)
        case "warning": return Qt.rgba(245/255, 158/255, 11/255, 0.18)
        case "danger": return Qt.rgba(239/255, 68/255, 68/255, 0.18)
        case "proxy": return Qt.rgba(0, 200/255, 255/255, 0.16)
        case "direct": return Qt.rgba(16/255, 185/255, 129/255, 0.18)
        default: return Qt.rgba(1, 1, 1, 0.08)
        }
    }

    readonly property color foregroundColor: {
        switch (tone) {
        case "accent": return "#00C8FF"
        case "success": return "#10B981"
        case "warning": return "#F59E0B"
        case "danger": return "#EF4444"
        case "proxy": return "#00C8FF"
        case "direct": return "#10B981"
        default: return FBLinkStyle.color.lightGray
        }
    }

    implicitHeight: compact ? 24 : 28
    implicitWidth: row.implicitWidth + (compact ? 14 : 18)
    radius: implicitHeight / 2
    color: backgroundColor
    border.color: Qt.rgba(foregroundColor.r, foregroundColor.g, foregroundColor.b, 0.18)
    border.width: 1

    RowLayout {
        id: row
        anchors.centerIn: parent
        spacing: 6

        Image {
            visible: root.iconSource !== ""
            source: root.iconSource
            sourceSize: Qt.size(root.compact ? 10 : 12, root.compact ? 10 : 12)

            layer.enabled: true
            layer.effect: ColorOverlay {
                color: root.foregroundColor
            }
        }

        LabelTextType {
            text: root.text
            font.pixelSize: root.compact ? 10 : 11
            font.weight: 700
            color: root.foregroundColor
        }
    }
}
