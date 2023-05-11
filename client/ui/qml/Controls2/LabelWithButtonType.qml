import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property string text
    property string descriptionText

    property var onClickedFunc

    property alias buttonImage: button.image
    property string iconImage

    implicitWidth: content.implicitWidth
    implicitHeight: content.implicitHeight

    RowLayout {
        id: content
        anchors.fill: parent

        Image {
            id: icon
            source: iconImage
            visible: iconImage ? true : false
            Layout.rightMargin: visible ? 16 : 0
        }

        ColumnLayout {
            Text {
                font.family: "PT Root UI VF"
                font.styleName: "normal"
                font.pixelSize: 18
                color: "#d7d8db"
                text: root.text
                wrapMode: Text.WordWrap

                Layout.fillWidth: true
                height: 22

                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
            }

            Text {
                font.family: "PT Root UI VF"
                font.styleName: "normal"
                font.pixelSize: 13
                font.letterSpacing: 0.02
                color: "#878B91"
                text: root.descriptionText
                wrapMode: Text.WordWrap

                visible: root.descriptionText !== ""

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
