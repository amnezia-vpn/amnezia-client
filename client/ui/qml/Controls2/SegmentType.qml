import QtQuick
import QtQuick.Controls

import Style 1.0

import "TextTypes"

RadioButton {
    id: root

    enum Size { Large, Medium, Small }

    property int size: SegmentType.Large

    implicitWidth: implicitContentWidth + leftPadding + rightPadding
    implicitHeight: root.size === SegmentType.Large ? 48 : (root.size === SegmentType.Medium ? 40 : 30)

    horizontalPadding: 12
    verticalPadding: 0

    checkable: false
    hoverEnabled: true

    opacity: root.enabled ? 1 : 0.4

    indicator: null

    background: Rectangle {
        radius: 16

        color: root.checked ? AmneziaStyle.color.bgSurfaceAlt : AmneziaStyle.color.transparent
        border.width: root.checked ? 1 : 0
        border.color: AmneziaStyle.color.accentLight

        Rectangle {
            anchors.fill: parent

            radius: parent.radius
            color: root.down ? AmneziaStyle.color.bgStateLayerPressed : AmneziaStyle.color.bgStateLayer
            opacity: !root.checked && root.enabled && (root.hovered || root.down) ? 1 : 0

            Behavior on opacity {
                NumberAnimation { duration: 150 }
            }
        }

        FocusRingType {
            radius: 16
            visible: root.visualFocus
        }
    }

    contentItem: BodyTextType {
        text: root.text
        color: AmneziaStyle.color.textPrimary

        lineHeight: (root.size === SegmentType.Small ? 18 : 24) + LanguageUiController.getLineHeightAppend()
        font.pixelSize: root.size === SegmentType.Small ? 14 : 16
        font.weight: root.size === SegmentType.Small ? 600 : 400

        wrapMode: Text.NoWrap
        elide: Text.ElideRight
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    HoverHandler {
        cursorShape: Qt.PointingHandCursor
    }

    Keys.onEnterPressed: function(event) {
        if (!event.isAutoRepeat && !root.checked) {
            root.clicked()
        }
    }

    Keys.onReturnPressed: function(event) {
        if (!event.isAutoRepeat && !root.checked) {
            root.clicked()
        }
    }
}
