import QtQuick

import Style 1.0

import "TextTypes"

Rectangle {
    id: root

    property alias text: label.text
    property color accentColor: AmneziaStyle.color.accentWarning

    implicitWidth: label.implicitWidth + 16
    implicitHeight: 22

    radius: 8
    color: Qt.alpha(root.accentColor, 0.12)
    border.color: root.accentColor
    border.width: 1

    AppCaptionTextType {
        id: label

        anchors.centerIn: parent
    }
}
