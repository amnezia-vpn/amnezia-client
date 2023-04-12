import QtQuick
import QtQuick.Layouts

ColumnLayout {
    id: root

    property string buttonImage
    property string headerText
    property var wrapMode

    ImageButtonType {
        id: button

        Layout.leftMargin: -6

        hoverEnabled: false
        image: root.buttonImage
        onClicked: {
            if (onClickedFunc && typeof onClickedFunc === "function") {
                onClickedFunc()
            }
        }
    }

    Text {
        id: header

        text: root.headerText

        color: "#D7D8DB"
        font.pixelSize: 36
        font.weight: 700
        font.family: "PT Root UI VF"
        font.letterSpacing: -0.03

        wrapMode: Text.WordWrap

        height: 38
    }
}
