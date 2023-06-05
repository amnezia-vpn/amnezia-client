import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Switch {
    id: root

    property string checkedIndicatorColor: "#412102"
    property string defaultIndicatorColor: "transparent"
    property string checkedIndicatorBorderColor: "#412102"
    property string defaultIndicatorBorderColor: "#494B50"

    property string checkedInnerCircleColor: "#FBB26A"
    property string defaultInnerCircleColor: "#D7D8DB"

    property string hoveredIndicatorBackgroundColor: Qt.rgba(1, 1, 1, 0.08)
    property string defaultIndicatorBackgroundColor: "transparent"

    indicator: Rectangle {
        implicitWidth: 52
        implicitHeight: 32
        x: content.width - width
        radius: 16
        color: root.checked ? checkedIndicatorColor : defaultIndicatorColor
        border.color: root.checked ? checkedIndicatorBorderColor : defaultIndicatorBorderColor

        Behavior on color {
            PropertyAnimation { duration: 200 }
        }
        Behavior on border.color {
            PropertyAnimation { duration: 200 }
        }

        Rectangle {
            id: innerCircle

            anchors.verticalCenter: parent.verticalCenter
            x: root.checked ? parent.width - width - 4 : 8
            width: root.checked ? 24 : 16
            height: root.checked ? 24 : 16
            radius: 23
            color: root.checked ? checkedInnerCircleColor : defaultInnerCircleColor

            Behavior on x {
                PropertyAnimation { duration: 200 }
            }
        }

        Rectangle {
            anchors.centerIn: innerCircle
            width: 40
            height: 40
            radius: 23
            color: hovered ? hoveredIndicatorBackgroundColor : defaultIndicatorBackgroundColor

            Behavior on color {
                PropertyAnimation { duration: 200 }
            }
        }
    }

    contentItem: ColumnLayout {
        id: content

        Text {
            text: root.text
            color: "#D7D8DB"
            font.pixelSize: 18
            font.weight: 400
            font.family: "PT Root UI VF"

            height: 22
            Layout.fillWidth: true
            Layout.bottomMargin: 16
        }
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        enabled: false
    }
}
