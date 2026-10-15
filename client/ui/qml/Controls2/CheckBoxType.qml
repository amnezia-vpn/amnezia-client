import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts

import Style 1.0

import "TextTypes"

CheckBox {
    id: root

    property string descriptionText

    implicitWidth: implicitContentWidth + leftPadding + rightPadding
    implicitHeight: Math.max(implicitContentHeight, implicitIndicatorHeight) + topPadding + bottomPadding

    topPadding: 8
    bottomPadding: 8
    leftPadding: implicitIndicatorWidth + spacing
    rightPadding: 0
    spacing: 12

    hoverEnabled: true
    focusPolicy: Qt.TabFocus

    opacity: root.enabled ? 1 : 0.4

    indicator: Item {
        x: 0
        y: (root.height - height) / 2

        implicitWidth: 40
        implicitHeight: 40

        Rectangle {
            anchors.fill: parent

            radius: 12
            visible: root.enabled && (root.hovered || root.down)
            color: root.checked ? AmneziaStyle.color.bgStateLayerAccent : AmneziaStyle.color.bgStateLayer
        }

        Rectangle {
            id: box

            anchors.centerIn: parent

            width: 24
            height: 24
            radius: 4

            color: AmneziaStyle.color.transparent
            border.width: 1
            border.color: root.checked ? AmneziaStyle.color.accentDark : AmneziaStyle.color.borderBold

            Behavior on border.color {
                ColorAnimation { duration: 150 }
            }
        }

        IconImage {
            id: check

            anchors.centerIn: box

            width: 24
            height: 24

            visible: root.checked
            source: "qrc:/images/controls/check.svg"
            sourceSize: Qt.size(24, 24)
            color: AmneziaStyle.color.accentLight
        }

        FocusRingType {
            target: box
            radius: 4
            visible: root.visualFocus
        }
    }

    contentItem: ColumnLayout {
        spacing: 4

        H3TextType {
            Layout.fillWidth: true

            visible: text !== ""
            text: root.text
        }

        SmallTextType {
            Layout.fillWidth: true

            visible: text !== ""
            text: root.descriptionText
        }
    }

    HoverHandler {
        cursorShape: Qt.PointingHandCursor
    }

    Keys.onEnterPressed: function(event) {
        if (!event.isAutoRepeat) {
            root.toggle()
            root.toggled()
        }
    }

    Keys.onReturnPressed: function(event) {
        if (!event.isAutoRepeat) {
            root.toggle()
            root.toggled()
        }
    }
}
