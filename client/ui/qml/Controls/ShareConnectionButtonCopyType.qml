import QtQuick 2.12
import QtQuick.Controls 2.12

ShareConnectionButtonType {
    readonly property string start_text: qsTr("Copy")
    readonly property string end_text: qsTr("Copied")

    Timer {
        id: timer
        interval: 1000; running: false; repeat: false
        onTriggered: text = start_text
    }

    text: start_text

    onClicked: {
        text = end_text
        timer.running = true
    }
}
