import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: root

    property string questionText
    property string yesText: "yes"
    property string noText: "no"
    property var yesFunc
    property var noFunc

    anchors.centerIn: Overlay.overlay
    modal: true
    closePolicy: Popup.NoAutoClose

    width: parent.width - 20

    ColumnLayout {
        width: parent.width
        Text {
            horizontalAlignment: Text.AlignHCenter
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            font.pixelSize: 16
            text: questionText
        }

        RowLayout {
            BlueButtonType {
                id: yesButton
                Layout.preferredWidth: parent.width / 2
                Layout.fillWidth: true
                text: yesText
                onClicked: {
                    root.enabled = false
                    if (yesFunc && typeof yesFunc === "function") {
                        yesFunc()
                    }
                    root.enabled = true
                }
            }
            BlueButtonType {
                id: noButton
                Layout.preferredWidth: parent.width / 2
                Layout.fillWidth: true
                text: noText
                onClicked: {
                    if (noFunc && typeof noFunc === "function") {
                        noFunc()
                    }
                }
            }
        }
    }
}
