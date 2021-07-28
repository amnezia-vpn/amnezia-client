import QtQuick 2.12
import QtQuick.Controls 2.12
import "./"
import QtGraphicalEffects 1.12

Item {
    id: root
    width: GC.screenWidth
    height: GC.screenHeight
    ImageButtonType {
        id: back
        x: 10
        y: 10
        width: 26
        height: 20
        icon.source: "qrc:/images/arrow_left.png"
        onClicked: {
            UiLogic.closePage()
        }
    }
    Text {
        font.family: "Lato"
        font.styleName: "normal"
        font.pixelSize: 24
        color: "#100A44"
        horizontalAlignment: Text.AlignRight
        verticalAlignment: Text.AlignVCenter
        text: qsTr("Servers list")
        x: 50
        y: 30
        width: 171
        height: 40
    }
    ImageButtonType {
        x: 240
        y: 39
        width: 24
        height: 24
        icon.source: "qrc:/images/plus.png"
    }
    ListModel {
        id: md
        ListElement {
            description: "Bill Smith"
            address: "555 3264"
        }
        ListElement {
            description: "John Brown"
            address: "555 8426"
        }
        ListElement {
            description: "Sam Wise"
            address: "555 0473"
        }
    }

    ListView {
        id: listWidget_servers
        x: 20
        y: 90
        width: 340
        height: 501
        model: md
        spacing: 5
        delegate: Item {
            height: 60
            width: 341
            LinearGradient {
                visible: !ms.containsMouse
                anchors.fill: parent
                start: Qt.point(0, 0)
                end: Qt.point(0, height)
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#FAFBFE" }
                    GradientStop { position: 1.0; color: "#ECEEFF" }
                }
            }
            LinearGradient {
                visible: ms.containsMouse
                anchors.fill: parent
                start: Qt.point(0, 0)
                end: Qt.point(0, height)
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#FAFBFE" }
                    GradientStop { position: 1.0; color: "#DCDEDF" }
                }
            }
            LabelType {
                id: label_address
                x: 20
                y: 40
                width: 141
                height: 16
                text: address
            }
            Text {
                x: 10
                y: 10
                width: 181
                height: 21
                font.family: "Lato"
                font.styleName: "normal"
                font.pixelSize: 16
                font.bold: true
                color: "#181922"
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
                wrapMode: Text.Wrap
                text: description
            }
            ImageButtonType {
                x: 212
                y: 25
                width: 32
                height: 24
                checkable: true
                iconMargin: 0
                icon.source: checked ? "qrc:/images/connect_button_connected.png"
                                     : "qrc:/images/connect_button_disconnected.png"
            }
            ImageButtonType {
                x: 300
                y: 25
                width: 24
                height: 24
                checkable: true
                icon.source: checked ? "qrc:/images/check.png"
                                     : "qrc:/images/uncheck.png"
            }
            ImageButtonType {
                x: 260
                y: 25
                width: 24
                height: 24
                icon.source: "qrc:/images/settings.png"
            }
            MouseArea {
                id: ms
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    listWidget_servers.currentIndex = index
                    mouse.accepted = false
                }
            }
        }
    }
}
