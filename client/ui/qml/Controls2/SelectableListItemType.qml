import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

RadioDelegate {
    id: root

    property string subtitle
    property bool showSeparator: true

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(56, implicitContentHeight + topPadding + bottomPadding)

    padding: 12
    leftPadding: 16
    verticalPadding: 14

    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    opacity: enabled ? 1.0 : 0.4

    Accessible.name: root.text
    Accessible.description: root.subtitle

    Keys.onEnterPressed: root.click()
    Keys.onReturnPressed: root.click()

    indicator: null

    background: Item {
        implicitWidth: 358
        implicitHeight: 56

        Rectangle {
            anchors.fill: parent

            radius: 16
            color: root.enabled && !root.checked && (root.hovered || root.down) ? (root.down ? AmneziaStyle.color.bgStateLayerPressed : AmneziaStyle.color.bgStateLayer)
                                                                                : AmneziaStyle.color.transparent
            border.width: root.checked || root.down ? 1 : 0
            border.color: root.checked ? AmneziaStyle.color.accentLight : AmneziaStyle.color.borderBold

            Behavior on color {
                ColorAnimation { duration: 150 }
            }
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top

            height: 1
            visible: root.showSeparator && !root.checked && !root.down
            color: AmneziaStyle.color.borderSoft
        }

        FocusRingType {
            radius: 16
            visible: root.visualFocus
        }

        HoverHandler {
            cursorShape: Qt.PointingHandCursor
            enabled: root.enabled
        }
    }

    contentItem: ListItemLabelType {
        title: root.text
        subtitle: root.subtitle
        maximumLineCount: 3
    }
}
