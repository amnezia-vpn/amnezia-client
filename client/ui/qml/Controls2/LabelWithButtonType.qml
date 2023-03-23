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

            image: buttonImage
            onClicked: {
                if (onClickedFunc && typeof onClickedFunc === "function") {
                    onClickedFunc()
                }
            }

            Layout.alignment: Qt.AlignRight
        }
    }

}
