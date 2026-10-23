import QtQuick

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

    SelectionGlowType {
        anchors.fill: parent

        source: dot
        visible: root.checked
        color: AmneziaStyle.color.accentLight
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
