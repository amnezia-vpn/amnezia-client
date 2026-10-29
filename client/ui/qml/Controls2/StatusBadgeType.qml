import QtQuick
import QtQuick.Controls

import Style 1.0

import "TextTypes"

Control {
    id: root

    enum Variant { Neutral, Success, Warning, Destructive, Recommendation }

    property int variant: StatusBadgeType.Warning
    property bool dot: false
    property string text

    readonly property color statusColor: {
        switch (root.variant) {
        case StatusBadgeType.Success: return AmneziaStyle.color.statusSuccess
        case StatusBadgeType.Warning: return AmneziaStyle.color.statusWarning
        case StatusBadgeType.Destructive: return AmneziaStyle.color.statusDestructive
        case StatusBadgeType.Recommendation: return AmneziaStyle.color.promoRecommendation
        default: return AmneziaStyle.color.borderMedium
        }
    }

    leftPadding: 8
    rightPadding: 8
    topPadding: 4
    bottomPadding: 4

    background: Rectangle {
        radius: 8
        color: {
            if (root.variant === StatusBadgeType.Recommendation) {
                return root.statusColor
            }
            return root.dot ? Qt.alpha(root.statusColor, 0.1) : AmneziaStyle.color.bgStateLayer
        }
        border.width: root.variant === StatusBadgeType.Recommendation ? 0 : 1
        border.color: root.statusColor
    }

    contentItem: Row {
        spacing: root.dot ? 0 : 4

        Item {
            anchors.verticalCenter: parent.verticalCenter

            width: 12
            height: 12

            visible: root.dot

            Rectangle {
                anchors.centerIn: parent

                width: 6
                height: 6
                radius: 3

                color: root.statusColor
            }
        }

        CaptionTextType {
            anchors.verticalCenter: parent.verticalCenter

            text: root.text
            wrapMode: Text.NoWrap
            font.letterSpacing: -0.4
            color: {
                if (root.variant === StatusBadgeType.Recommendation) {
                    return AmneziaStyle.color.textInverted
                }
                return root.dot ? root.statusColor : AmneziaStyle.color.textPrimary
            }
        }
    }
}
