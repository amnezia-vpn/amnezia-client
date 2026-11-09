import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "TextTypes"

AbstractButton {
    id: root

    property string subtitle
    property string trailingText
    property string badgeText

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(72, implicitContentHeight + topPadding + bottomPadding)

    padding: 16
    spacing: 12

    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    opacity: enabled ? 1.0 : 0.4

    Accessible.role: Accessible.RadioButton
    Accessible.checkable: true
    Accessible.checked: root.checked
    Accessible.name: root.text
    Accessible.description: root.subtitle

    Keys.onEnterPressed: root.click()
    Keys.onReturnPressed: root.click()

    background: Rectangle {
        implicitWidth: 358
        implicitHeight: 72

        radius: 16
        color: root.enabled && !root.checked && (root.hovered || root.down) ? AmneziaStyle.color.bgStateLayer
                                                                            : AmneziaStyle.color.transparent
        border.width: 1
        border.color: {
            if (root.checked) {
                return AmneziaStyle.color.accentLight
            }
            if (root.down) {
                return AmneziaStyle.color.borderBold
            }
            return AmneziaStyle.color.borderSoft
        }

        Behavior on color {
            ColorAnimation { duration: 150 }
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

    contentItem: RowLayout {
        spacing: root.spacing

        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter

            spacing: 4

            H3TextType {
                Layout.fillWidth: true

                text: root.text
                color: root.checked ? AmneziaStyle.color.textAccent : AmneziaStyle.color.textPrimary
                elide: Text.ElideRight
                maximumLineCount: 2
            }

            CaptionTextType {
                Layout.fillWidth: true

                visible: text !== ""
                text: root.subtitle
            }
        }

        ColumnLayout {
            Layout.alignment: Qt.AlignTop | Qt.AlignRight

            spacing: 4
            visible: root.trailingText !== "" || root.badgeText !== ""

            SmallTextType {
                Layout.alignment: Qt.AlignRight

                visible: text !== ""
                text: root.trailingText
                color: root.checked ? AmneziaStyle.color.textAccent : AmneziaStyle.color.textPrimary
                horizontalAlignment: Text.AlignRight
                wrapMode: Text.NoWrap
            }

            RecommendationBadgeType {
                Layout.alignment: Qt.AlignRight

                visible: root.badgeText !== ""
                text: root.badgeText
            }
        }
    }
}
