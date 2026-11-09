import QtQuick
import QtQuick.Controls

import Style 1.0

BusyIndicator {
    id: root

    enum Size { Small, Regular, Medium, Large }

    property int size: SpinnerType.Medium
    property color color: AmneziaStyle.color.textPrimary

    padding: 0

    QtObject {
        id: d

        readonly property int box: {
            switch (root.size) {
            case SpinnerType.Small: return 16
            case SpinnerType.Regular: return 24
            case SpinnerType.Large: return 44
            default: return 32
            }
        }
        property int step: 0
    }

    contentItem: Item {
        implicitWidth: d.box
        implicitHeight: d.box

        visible: root.running

        Repeater {
            model: 8

            Rectangle {
                id: spoke

                required property int index

                x: (d.box - width) / 2
                y: d.box / 32

                width: d.box / 8
                height: d.box * 10 / 32
                radius: width / 2

                color: root.color
                opacity: 1 - ((d.step - index + 8) % 8) / 8

                transform: Rotation {
                    origin.x: spoke.width / 2
                    origin.y: d.box * 15 / 32
                    angle: spoke.index * 45
                }
            }
        }

        Timer {
            interval: 125
            repeat: true
            running: root.running && root.visible
            onTriggered: d.step = (d.step + 1) % 8
        }
    }
}
