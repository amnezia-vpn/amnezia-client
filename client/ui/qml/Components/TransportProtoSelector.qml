import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../Controls2"
import "../Controls2/TextTypes"

Rectangle {
    id: root

    property real rootWidth: root.width
    property int currentIndex

    implicitWidth: transportProtoButtonGroup.implicitWidth
    implicitHeight: transportProtoButtonGroup.implicitHeight

    color: "#1C1D21"
    radius: 16

    RowLayout {
        id: transportProtoButtonGroup

        spacing: 0

        HorizontalRadioButton {
            checked: root.currentIndex === 0

            implicitWidth: (rootWidth - 32) / 2
            text: "UDP"

            onClicked: {
                root.currentIndex = 0
            }
        }

        HorizontalRadioButton {
            checked: root.currentIndex === 1

            implicitWidth: (rootWidth - 32) / 2
            text: "TCP"

            onClicked: {
                root.currentIndex = 1
            }
        }
    }
}
