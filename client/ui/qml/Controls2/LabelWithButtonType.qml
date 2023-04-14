import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property string text
    property string descriptionText

    property var onClickedFunc
    property alias buttonImage : button.image

    implicitWidth: content.implicitWidth
    implicitHeight: content.implicitHeight

    RowLayout {
        id: content
        anchors.fill: parent

        ColumnLayout {
            Text {
                font.family: "PT Root UI"
                font.styleName: "normal"
                font.pixelSize: 18
                color: "#d7d8db"
                text: root.text

                Layout.fillWidth: true
                height: 22

                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
            }

            Text {
                font.family: "PT Root UI"
                font.styleName: "normal"
                font.pixelSize: 13
                font.letterSpacing: 0.02
                color: "#878B91"
                text: root.descriptionText
                wrapMode: Text.WordWrap

                Layout.fillWidth: true
                height: 16

                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
            }
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
