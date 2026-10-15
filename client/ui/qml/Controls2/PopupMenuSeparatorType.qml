import QtQuick
import QtQuick.Layouts

import Style 1.0

Item {
    Layout.fillWidth: true

    implicitHeight: 8

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: 8
        anchors.rightMargin: 8

        height: 1
        color: AmneziaStyle.color.borderSoft
    }
}
