import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: root

    property string popupWarningText

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
            text: root.popupWarningText
        }

        BlueButtonType {
            Layout.preferredWidth: parent.width / 2
            Layout.fillWidth: true
            text: "Continue"
            onClicked: {
                root.close()
            }
        }
    }
}
