import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts

import Style 1.0

import "TextTypes"

Control {
    id: root

    enum Variant { Info, Warning, Error }

    property int variant: NotificationType.Info
    property bool compact: false

    property string text
    property int textFormat: Text.PlainText
    property string iconSource: {
        switch (root.variant) {
        case NotificationType.Warning: return "qrc:/images/controls/alert-triangle.svg"
        case NotificationType.Error: return "qrc:/images/controls/alert-circle.svg"
        default: return "qrc:/images/controls/info.svg"
        }
    }

    padding: root.compact ? 12 : 16

    background: Rectangle {
        radius: 16
        color: AmneziaStyle.color.bgSurface
    }

    contentItem: RowLayout {
        spacing: root.compact ? 8 : 12

        IconImage {
            readonly property int size: root.compact ? 18 : 24

            Layout.alignment: Qt.AlignTop
            Layout.preferredWidth: size
            Layout.preferredHeight: size

            visible: root.iconSource !== ""
            source: root.iconSource
            sourceSize: Qt.size(size, size)
            color: {
                switch (root.variant) {
                case NotificationType.Warning: return AmneziaStyle.color.statusWarning
                case NotificationType.Error: return AmneziaStyle.color.statusDestructive
                default: return AmneziaStyle.color.iconDefault
                }
            }
        }

        SmallTextType {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter

            text: root.text
            textFormat: root.textFormat
            color: root.variant === NotificationType.Error ? AmneziaStyle.color.statusDestructive
                                                           : AmneziaStyle.color.textPrimary

            onLinkActivated: function(link) {
                Qt.openUrlExternally(link)
            }

            HoverHandler {
                cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
            }
        }
    }
}
