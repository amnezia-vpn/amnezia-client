import QtQuick

Item {
    id: root

    property Item source
    property color color

    ShadowType {
        x: root.source ? root.source.x : 0
        y: root.source ? root.source.y : 0
        width: root.source ? root.source.width : 0
        height: root.source ? root.source.height : 0

        source: root.source
        color: root.color
        blur: 14.592111587524414
    }

    ShadowType {
        x: root.source ? root.source.x : 0
        y: root.source ? root.source.y : 0
        width: root.source ? root.source.width : 0
        height: root.source ? root.source.height : 0

        source: root.source
        color: root.color
        blur: 8
    }

    ShadowType {
        x: root.source ? root.source.x : 0
        y: root.source ? root.source.y : 0
        width: root.source ? root.source.width : 0
        height: root.source ? root.source.height : 0

        source: root.source
        color: root.color
        blur: 4
    }
}
