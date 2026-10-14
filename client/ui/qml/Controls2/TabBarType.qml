import QtQuick
import QtQuick.Controls

import Style 1.0

TabBar {
    id: root

    spacing: 8

    topPadding: 8
    bottomPadding: 4 + PageController.safeAreaBottomMargin
    leftPadding: 16
    rightPadding: 16

    background: Rectangle {
        color: AmneziaStyle.color.bgSurfaceAlt

        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            height: 1
            color: AmneziaStyle.color.borderSoft
        }
    }
}
