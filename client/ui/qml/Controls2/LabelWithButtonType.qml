import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property string text

    property var onClickedFunc
    property alias buttonImage : button.image

    implicitWidth: 360
    implicitHeight: 72

    RowLayout {
        anchors.fill: parent

        Text {
            font.family: "PT Root UI"
            font.styleName: "normal"
            font.pixelSize: 18
            color: "#d7d8db"
            text: root.text
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
        }

        ImageButtonType {
            id: button

            hoverEnabled: false
            image: buttonImage
            onClicked: {
                if (onClickedFunc && typeof onClickedFunc === "function") {
                    onClickedFunc()
                }
            }

            Layout.alignment: Qt.AlignRight

            Rectangle {
                id: imageBackground
                anchors.fill: button
                radius: 12
                color: "transparent"


                Behavior on color {
                    PropertyAnimation { duration: 200 }
                }
            }
        }
    }
    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        hoverEnabled: true

        onEntered: {
            imageBackground.color = button.hoveredColor
        }

        onExited: {
            imageBackground.color = button.defaultColor
        }

        onPressedChanged: {
            imageBackground.color = pressed ? button.pressedColor : entered ? button.hoveredColor : button.defaultColor
        }

        onClicked: {
            button.clicked()
        }
    }
}
