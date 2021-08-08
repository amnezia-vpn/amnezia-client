import QtQuick 2.12
import QtQuick.Controls 2.12
import "./"
import Qt.labs.platform 1.0
import QtQuick.Dialogs 1.0

Item {
    id: root
    width: GC.screenWidth
    height: GC.screenHeight
    Text {
        font.family: "Lato"
        font.styleName: "normal"
        font.pixelSize: 16
        color: "#333333"
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        text: qsTr("Web site/Hostname/IP address/Subnet")
        x: 20
        y: 110
        width: 311
        height: 21
    }
    Text {
        font.family: "Lato"
        font.styleName: "normal"
        font.pixelSize: 20
        color: "#100A44"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignTop
        text: UiLogic.labelSitesAddCustomText
        x: 20
        y: 40
        width: 340
        height: 60
    }
    TextFieldType {
        x: 20
        y: 140
        width: 231
        height: 31
        placeholderText: qsTr("yousite.com or IP address")
        text: UiLogic.lineEditSitesAddCustomText
        onEditingFinished: {
            UiLogic.lineEditSitesAddCustomText = text
        }
        onAccepted: {
            UiLogic.onPushButtonAddCustomSitesClicked()
        }
    }
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
    BlueButtonType {
        id: sites_add
        x: 260
        y: 140
        width: 51
        height: 31
        font.pixelSize: 24
        text: "+"
        onClicked: {
            UiLogic.onPushButtonAddCustomSitesClicked()
        }
    }
    BlueButtonType {
        id: sites_delete
        x: 80
        y: 589
        width: 231
        height: 31
        font.pixelSize: 16
        text: qsTr("Delete selected")
        onClicked: {
            UiLogic.onPushButtonSitesDeleteClicked(tb.currentRow)
        }
    }

    BasicButtonType {
        id: sites_import
        x: 320
        y: 140
        width: 51
        height: 31
        background: Rectangle {
            anchors.fill: parent
            radius: 4
            color: parent.containsMouse ? "#211966" : "#100A44"
        }
        font.pixelSize: 16
        contentItem: Item {
            anchors.fill: parent
            Image {
                anchors.centerIn: parent
                width: 20
                height: 20
                source: "qrc:/images/folder.png"
                fillMode: Image.Stretch
            }
        }
        antialiasing: true
        onClicked: {
            fileDialog.open()
        }
    }
    FileDialog {
        id: fileDialog
        title: qsTr("Import IP addresses")
        visible: false
        folder: StandardPaths.writableLocation(StandardPaths.DocumentsLocation)
        onAccepted: {
            UiLogic.onPushButtonSitesImportClicked(fileUrl)
        }
    }
    TableView {
        id: tb
        x: 20
        y: 200
        width: 341
        height: 371
        rowSpacing: 1
        clip: true
        property int currentRow: -1
        columnSpacing: 0
        model: UiLogic.tableViewSitesModel

        delegate: Item {
            implicitWidth: 170
            implicitHeight: 30
            Rectangle {
                anchors.top: parent.top
                width: parent.width
                height: 1
                color: "lightgray"
                visible: row !== tb.currentRow
            }
            Rectangle {
                anchors.fill: parent
                color: "#63B4FB"
                visible: row === tb.currentRow

            }

            Text {
                text: display
                anchors.fill: parent
                leftPadding: 10
                verticalAlignment: Text.AlignVCenter
            }
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    tb.currentRow = row
                }
            }
        }
    }
}
