import QtQuick
import QtQuick.Layouts

import Style 1.0

Rectangle {
    id: root

    property bool filled: false
    property alias spacing: column.spacing

    default property alias content: column.data

    implicitHeight: column.implicitHeight + 8 + 8 + PageController.safeAreaBottomMargin

    color: root.filled ? AmneziaStyle.color.bgSurfaceAlt : AmneziaStyle.color.bgBase

    ColumnLayout {
        id: column

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 8
        anchors.leftMargin: 16
        anchors.rightMargin: 16

        spacing: 12
    }
}
