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

    property Item blurSource

    background: BackdropBlurType {
        sourceItem: root.blurSource
        blurRadius: 20
        radius: 16
        tintColor: Qt.alpha(AmneziaStyle.color.bgOverlay, 0.75)
        borderColor: AmneziaStyle.color.transparent
    }

    contentItem: SpinnerType {
        size: SpinnerType.Medium
        running: root.visible
    }
}
