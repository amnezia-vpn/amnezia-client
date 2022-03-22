import QtQuick 2.12

LabelType {
    id: label_connection_code
    width: parent.width - 60
    x: 30
    font.pixelSize: 14
    textFormat: Text.RichText
    onLinkActivated: Qt.openUrlExternally(link)

    MouseArea {
        anchors.fill: parent
        cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
        acceptedButtons: Qt.NoButton
    }
}

