import QtQuick
import QtQuick.Controls

import Style 1.0

import "TextTypes"

AbstractButton {
    id: root

    property color textColor: AmneziaStyle.color.textTertiary
    property bool emphasized: false

    implicitWidth: label.implicitWidth + 24
    implicitHeight: 48

    hoverEnabled: true

    background: Rectangle {
        radius: 16
        color: root.down ? AmneziaStyle.color.bgStateLayerPressed : (root.hovered ? AmneziaStyle.color.bgStateLayer : AmneziaStyle.color.transparent)

        Behavior on color {
            ColorAnimation { duration: 150 }
        }

        FocusRingType {
            radius: 16
            visible: root.visualFocus
        }
    }

    contentItem: BodyTextType {
        id: label

        text: root.text
        color: root.textColor
        font.weight: root.emphasized ? 600 : 400
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        wrapMode: Text.NoWrap
        elide: Text.ElideRight
    }

    HoverHandler {
        cursorShape: Qt.PointingHandCursor
    }

    Keys.onEnterPressed: root.clicked()
    Keys.onReturnPressed: root.clicked()
}
