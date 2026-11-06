import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts

import ServerQuality 1.0
import Style 1.0

import "../Controls2"

RadioItemType {
    id: root

    property bool showWarning: false
    property int latencyMs: -1
    property int latencyQuality: ServerQuality.Unknown
    property bool latencyTesting: false

    signal settingsRequested()

    ServerLatencyBadge {
        Layout.leftMargin: 8

        stacked: true
        latencyMs: root.latencyMs
        quality: root.latencyQuality
        testing: root.latencyTesting
    }

    Item {
        Layout.preferredWidth: 48
        Layout.preferredHeight: 48

        visible: root.showWarning

        IconImage {
            anchors.centerIn: parent
            width: 24
            height: 24

            source: "qrc:/images/controls/alert-circle.svg"
            sourceSize: Qt.size(24, 24)
            color: AmneziaStyle.color.statusWarning
        }
    }

    IconButtonType {
        icon.source: "qrc:/images/controls/settings.svg"
        Accessible.name: qsTr("Server settings")

        onClicked: root.settingsRequested()
    }
}
