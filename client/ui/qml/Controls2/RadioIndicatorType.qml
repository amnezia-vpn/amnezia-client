import QtQuick
import QtQuick.Effects

import Style 1.0

Item {
    id: root

    property bool checked: false

    implicitWidth: 24
    implicitHeight: 24

    Rectangle {
        anchors.fill: parent
        anchors.margins: 0.5

        radius: width / 2
        color: root.checked ? AmneziaStyle.color.bgBase : AmneziaStyle.color.transparent
        border.width: 1
        border.color: root.checked ? AmneziaStyle.color.accentDark : AmneziaStyle.color.iconDefault

        Behavior on border.color {
            ColorAnimation { duration: 150 }
        }
    }

    MultiEffect {
        anchors.fill: dot
        source: dot
        visible: root.checked
        shadowEnabled: true
        shadowColor: AmneziaStyle.color.accentLight
        shadowBlur: 0.6
        shadowHorizontalOffset: 0
        shadowVerticalOffset: 0
    }

    Rectangle {
        id: dot

        anchors.centerIn: parent
        width: 12
        height: 12

        radius: 6
        color: AmneziaStyle.color.accentLight
        visible: root.checked
    }
}
