import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "../Controls2"
import "../Controls2/TextTypes"

Rectangle {
    id: root

    property real rootWidth: root.width
    property int currentIndex

    implicitWidth: transportProtoButtonGroup.implicitWidth
    implicitHeight: transportProtoButtonGroup.implicitHeight

    color: AmneziaStyle.color.onyxBlack
    radius: 16

    onFocusChanged: {
        if (focus) {
            udpButton.forceActiveFocus()
        }
    }

    RowLayout {
        id: transportProtoButtonGroup

        spacing: 0

        HorizontalRadioButton {
            id: udpButton
            checked: root.currentIndex === 0

            hoverEnabled: root.enabled

            implicitWidth: (rootWidth - 32) / 2
            text: "UDP"

            onClicked: {
                root.currentIndex = 0
            }
        }

        HorizontalRadioButton {
            id: tcpButton
            checked: root.currentIndex === 1

            hoverEnabled: root.enabled

            implicitWidth: (rootWidth - 32) / 2
            text: "TCP"

            onClicked: {
                root.currentIndex = 1
            }
        }
    }
}
