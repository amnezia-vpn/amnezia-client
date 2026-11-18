import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "TextTypes"

Switch {
    id: root

    property string descriptionText

    readonly property bool hasLabel: root.text !== "" || root.descriptionText !== ""

    implicitWidth: root.hasLabel ? implicitContentWidth + leftPadding + rightPadding : implicitIndicatorWidth
    implicitHeight: Math.max(implicitContentHeight + topPadding + bottomPadding, implicitIndicatorHeight)

    padding: 0
    rightPadding: root.hasLabel ? implicitIndicatorWidth + 16 : 0
    spacing: 16

    hoverEnabled: true
    focusPolicy: Qt.TabFocus

    opacity: root.enabled ? 1 : 0.4

    indicator: Item {
        x: root.width - width
        y: (root.height - height) / 2

        implicitWidth: 50
        implicitHeight: 30

        Rectangle {
            anchors.fill: parent

            radius: height / 2
            color: AmneziaStyle.color.bgSurfaceAlt
            border.width: 1
            border.color: root.checked ? AmneziaStyle.color.accentDark : AmneziaStyle.color.borderSoft

            Behavior on border.color {
                ColorAnimation { duration: 150 }
            }
        }

        Rectangle {
            anchors.centerIn: knob

            width: 36
            height: 36
            radius: 18

            visible: root.enabled && (root.hovered || root.down)
            color: root.checked ? AmneziaStyle.color.bgStateLayerAccent : AmneziaStyle.color.bgStateLayer
        }

        SelectionGlowType {
            anchors.fill: parent

            source: knob
            visible: root.checked
            color: AmneziaStyle.color.accentLight
        }

        Rectangle {
            id: knob

            x: root.checked ? parent.width - width - 5 : 5
            y: (parent.height - height) / 2

            width: 20
            height: 20
            radius: 10

            color: root.checked ? AmneziaStyle.color.accentLight : AmneziaStyle.color.iconSecondary

            Behavior on x {
                enabled: !root.down
                NumberAnimation { duration: 150 }
            }

            Behavior on color {
                ColorAnimation { duration: 150 }
            }
        }

        FocusRingType {
            radius: 15
            visible: root.visualFocus
        }
    }

    contentItem: ColumnLayout {
        visible: root.hasLabel
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
