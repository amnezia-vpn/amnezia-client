import QtQuick 2.12
import QtQuick.Controls 2.12
import QtGraphicalEffects 1.12

Item {
    id: root
    property bool active: false
    property string text: ""
    height: active ? contentLoader.item.height + 40 + 5 * 2 : 40
    signal clicked()

    Rectangle {
        x: 0
        y: 0
        width: parent.width
        height: 40
        color: "transparent"
        clip: true
        radius: 2
        LinearGradient {
            anchors.fill: parent
            start: Qt.point(0, 0)
            end: Qt.point(0, height)
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#E1E1E1" }
                GradientStop { position: 0.4; color: "#DDDDDD" }
                GradientStop { position: 0.5; color: "#D8D8D8" }
                GradientStop { position: 1.0; color: "#D3D3D3" }
            }
        }
        Image {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 10
            source: "qrc:/images/share.png"
        }
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 2
            color: "#148CD2"
            visible: ms.containsMouse ? true : false
        }
        Text {
            x: 40
            anchors.verticalCenter: parent.verticalCenter
            font.family: "Lato"
            font.styleName: "normal"
            font.pixelSize: 18
            color: "#100A44"
            font.bold: true
            text: root.text
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.Wrap
        }
        MouseArea {
            id: ms
            anchors.fill: parent
            hoverEnabled: true
            onClicked: root.clicked()
        }
    }
}

