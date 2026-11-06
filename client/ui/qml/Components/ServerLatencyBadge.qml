import QtQuick
import QtQuick.Layouts

import ServerQuality 1.0
import Style 1.0

import "../Controls2"
import "../Controls2/TextTypes"

GridLayout {
    id: root

    property int latencyMs: -1
    property int quality: ServerQuality.Unknown
    property bool testing: false
    property bool stacked: false

    readonly property bool hasResult: !root.testing && root.quality !== ServerQuality.Unknown
    readonly property bool hasLatency: root.hasResult && root.latencyMs >= 0
    readonly property bool hasContent: root.testing || root.hasResult

    columns: root.stacked ? 1 : 2
    rowSpacing: 2
    columnSpacing: 8
    visible: root.hasContent

    SpinnerType {
        Layout.alignment: root.stacked ? Qt.AlignRight : Qt.AlignVCenter

        visible: root.testing
        running: root.testing
        size: SpinnerType.Small
    }

    StatusBadgeType {
        Layout.alignment: root.stacked ? Qt.AlignRight : Qt.AlignVCenter

        visible: root.hasResult
        dot: root.quality !== ServerQuality.NoReply
        variant: {
            switch (root.quality) {
            case ServerQuality.Excellent:
            case ServerQuality.Good: return StatusBadgeType.Success
            case ServerQuality.Fair: return StatusBadgeType.Warning
            case ServerQuality.NoReply: return StatusBadgeType.Neutral
            default: return StatusBadgeType.Destructive
            }
        }
        text: {
            if (root.hasLatency) {
                return qsTr("%1 ms").arg(root.latencyMs)
            }
            return root.quality === ServerQuality.NoReply ? qsTr("No reply") : qsTr("Unreachable")
        }
    }

    CaptionTextType {
        Layout.alignment: root.stacked ? Qt.AlignRight : Qt.AlignVCenter

        visible: root.hasLatency
        text: {
            switch (root.quality) {
            case ServerQuality.Excellent: return qsTr("Excellent")
            case ServerQuality.Good: return qsTr("Good")
            case ServerQuality.Fair: return qsTr("Fair")
            default: return qsTr("Poor")
            }
        }
        color: AmneziaStyle.color.textTertiary
    }
}
