import QtQuick
import Qt5Compat.GraphicalEffects

import Style 1.0

Item {
    id: root

    property Item sourceItem
    property real blurPadding: 40
    property real blurRadius: 50
    property color tintColor: Qt.alpha(AmneziaStyle.color.textStaticWhite, 0.10)
    property color borderColor: Qt.alpha(AmneziaStyle.color.textStaticWhite, 0.75)

    Item {
        anchors.fill: parent

        layer.enabled: true
        layer.effect: OpacityMask {
            maskSource: backdropMask
        }

        Item {
            x: -root.blurPadding
            y: -root.blurPadding
            width: root.width + root.blurPadding * 2
            height: root.height + root.blurPadding * 2

            ShaderEffectSource {
                id: backdrop

                anchors.fill: parent
                sourceItem: root.sourceItem
                sourceRect: {
                    if (!root.sourceItem) {
                        return Qt.rect(0, 0, 0, 0)
                    }
                    root.x; root.y; root.width; root.height; root.sourceItem.x; root.sourceItem.y;
                    var pos = root.mapToItem(root.sourceItem, -root.blurPadding, -root.blurPadding)
                    return Qt.rect(pos.x, pos.y, width, height)
                }
                visible: false
            }

            FastBlur {
                anchors.fill: parent
                source: backdrop
                radius: root.blurRadius
            }
        }
    }

    Rectangle {
        id: backdropMask

        anchors.fill: parent
        radius: height / 2
        visible: false
    }

    Rectangle {
        anchors.fill: parent
        radius: height / 2

        color: root.tintColor
        border.color: root.borderColor
        border.width: 1
    }
}
