import QtQuick
import QtQuick.Layouts

Item {
    id: root

    property string buttonImage
    property string headerText
    property string descriptionText

    implicitWidth: content.implicitWidth
    implicitHeight: content.implicitHeight

    ColumnLayout {
        id: content
        anchors.fill: parent

        ImageButtonType {
            id: backButton

            Layout.leftMargin: -6

            image: root.buttonImage
            imageColor: "#D7D8DB"
            onClicked: {
                UiLogic.closePage()
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
            Layout.fillWidth: true
        }

        Text {
            id: description

            text: root.descriptionText

            color: "#878B91"
            font.pixelSize: 16
            font.weight: 400
            font.family: "PT Root UI VF"
            font.letterSpacing: -0.03

            wrapMode: Text.WordWrap

            height: 24
            Layout.fillWidth: true
        }
    }
}
