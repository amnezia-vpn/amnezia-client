import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

Popup {
    id: root

    default property alias items: column.data

    property int openFocusReason: Qt.OtherFocusReason
    property Item openerItem: null

    function openAbove(anchorItem) {
        root.openerItem = anchorItem
        root.openFocusReason = anchorItem.visualFocus ? Qt.TabFocusReason : Qt.OtherFocusReason
        const point = anchorItem.mapToItem(root.parent, anchorItem.width, 0)
        root.x = Math.max(16, Math.round(point.x - root.width))
        root.y = Math.max(16, Math.round(point.y - root.implicitHeight - 8))
        root.open()
    }

    parent: Overlay.overlay

    width: Math.min(301, parent ? parent.width - 32 : 301)

    padding: 4

    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    Overlay.modal: Item {}

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 150 }
    }

    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 150 }
    }

    onOpened: {
        for (let i = 0; i < column.children.length; ++i) {
            const item = column.children[i]
            if (item.visible && item.activeFocusOnTab) {
                item.forceActiveFocus(root.openFocusReason)
                break
            }
        }
    }

    onClosed: {
        if (root.openerItem && root.openFocusReason === Qt.TabFocusReason) {
            root.openerItem.forceActiveFocus(Qt.TabFocusReason)
        }
    }

    background: Item {
        Rectangle {
            id: panelShadowSource

            anchors.fill: parent

            radius: 14
            color: AmneziaStyle.color.bgSurface
            visible: false
        }

        ShadowType {
            anchors.fill: panelShadowSource
            source: panelShadowSource
            verticalOffset: 8
            blur: 24
            color: AmneziaStyle.color.bgOverlay
        }

        Rectangle {
            anchors.fill: parent

            radius: 14
            color: AmneziaStyle.color.bgSurface
            border.width: 1
            border.color: AmneziaStyle.color.borderSoft
        }
    }

    contentItem: ColumnLayout {
        id: column

        spacing: 0
    }
}
