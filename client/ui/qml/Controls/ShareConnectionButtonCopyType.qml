import QtQuick
import QtQuick.Controls

ShareConnectionButtonType {
    property string start_text: qsTr("Copy")
    property string end_text: qsTr("Copied")

    property string copyText

    enabled: copyText.length > 0
    visible: copyText.length > 0

    Timer {
        id: timer
        interval: 1000; running: false; repeat: false
        onTriggered: text = start_text
    }

    text: start_text

    onClicked: {
        text = end_text
        timer.running = true
        UiLogic.copyToClipboard(copyText)
    }
}
