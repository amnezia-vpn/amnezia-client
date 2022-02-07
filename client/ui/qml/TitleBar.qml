import QtQuick 2.12
import QtQuick.Controls 2.12
import "./"
import "Config"

Rectangle {
    id: root
    color: "#F5F5F5"
    width: GC.screenWidth
    height: 30
    signal closeButtonClicked()

    Button {
        id: closeButton
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        anchors.rightMargin: 5
        icon.source: "qrc:/images/close.png"
        icon.width: 16
        icon.height: 16
        width: height
        height: 20
        background: Item {}
        contentItem: Image {
            source: closeButton.icon.source
            anchors.fill: closeButton
            anchors.margins: ms.containsMouse ? 3 : 4
        }
        MouseArea {
            id: ms
            hoverEnabled: true
            anchors.fill: closeButton
        }
        onClicked: {
            root.closeButtonClicked()
        }
    }
}
