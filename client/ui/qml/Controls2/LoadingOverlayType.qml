import QtQuick
import QtQuick.Controls

import Style 1.0

Popup {
    id: root

    anchors.centerIn: parent

    modal: true
    closePolicy: Popup.NoAutoClose

    visible: false

    padding: 16

    Overlay.modal: Rectangle {
        color: AmneziaStyle.color.bgOverlay
    }

    background: Rectangle {
        radius: 16
        color: AmneziaStyle.color.bgOverlay
    }

    contentItem: SpinnerType {
        size: SpinnerType.Medium
        running: root.visible
    }
}
