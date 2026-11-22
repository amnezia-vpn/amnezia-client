import QtQuick
import QtQuick.Controls.impl
import QtQuick.Layouts

import Style 1.0

import "../Controls2/TextTypes"

ColumnLayout {
    id: root

    readonly property bool showTrafficSpeed: ConnectionController.isConnected && ConnectionController.hasTrafficSpeed

    component TrafficRateLabel: RowLayout {
        property alias iconSource: rateIcon.source
        property alias text: rateText.text

        spacing: 4

        IconImage {
            id: rateIcon

            Layout.preferredWidth: 16
            Layout.preferredHeight: 16

            sourceSize: Qt.size(16, 16)
            color: AmneziaStyle.color.textTertiary
        }

        SmallTextType {
            id: rateText

            Layout.minimumWidth: widestRateMetrics.advanceWidth

            font.features: { "tnum": 1 }

            TextMetrics {
                id: widestRateMetrics

                font: rateText.font
                text: qsTr("%1 Mbps").arg("000.00")
            }
        }
    }

    spacing: 8
    visible: latencyBadge.hasContent || root.showTrafficSpeed

    ServerLatencyBadge {
        id: latencyBadge

        Layout.alignment: Qt.AlignHCenter

        latencyMs: ServerLatencyController.defaultServerLatencyMs
        quality: ServerLatencyController.defaultServerQuality
        testing: ServerLatencyController.isDefaultServerTesting
    }

    RowLayout {
        Layout.alignment: Qt.AlignHCenter

        visible: root.showTrafficSpeed
        spacing: 24

        TrafficRateLabel {
            iconSource: "qrc:/images/controls/chevron-down.svg"
            text: ConnectionController.downloadSpeed
            Accessible.name: qsTr("Download speed: %1").arg(text)
        }

        TrafficRateLabel {
            iconSource: "qrc:/images/controls/chevron-up.svg"
            text: ConnectionController.uploadSpeed
            Accessible.name: qsTr("Upload speed: %1").arg(text)
        }
    }
}
