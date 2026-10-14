import QtQuick
import QtQuick.Layouts

import Style 1.0

Rectangle {
    id: root

    default property alias rows: rowsColumn.data

    implicitHeight: Math.max(0, rowsColumn.implicitHeight - 1)

    radius: 16
    color: AmneziaStyle.color.bgSurface
    clip: true

    ColumnLayout {
        id: rowsColumn

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top

        spacing: 0
    }
}
