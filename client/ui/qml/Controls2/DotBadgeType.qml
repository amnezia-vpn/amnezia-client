import QtQuick

import Style 1.0

Item {
    id: root

    property color color: AmneziaStyle.color.promoRecommendation

    implicitWidth: 16
    implicitHeight: 16

    Rectangle {
        anchors.centerIn: parent

        width: 6
        height: 6
        radius: width / 2

        color: root.color
    }
}
