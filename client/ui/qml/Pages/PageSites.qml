import QtQuick 2.12
import QtQuick.Controls 2.12
import Qt.labs.platform 1.0
import QtQuick.Dialogs 1.0
import "./"
import "../Controls"
import "../Config"

Item {
    id: root
    BackButton {
        id: back
    }
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
        text: SitesLogic.labelSitesAddCustomText
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
        text: SitesLogic.lineEditSitesAddCustomText
        onEditingFinished: {
            SitesLogic.lineEditSitesAddCustomText = text
        }
        onAccepted: {
            SitesLogic.onPushButtonAddCustomSitesClicked()
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
            SitesLogic.onPushButtonAddCustomSitesClicked()
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
            SitesLogic.onPushButtonSitesDeleteClicked(tb.currentRow)
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
            SitesLogic.onPushButtonSitesImportClicked(fileUrl)
        }
    }
    ListView {
        id: tb
        x: 20
        y: 200
        width: 341
        height: 371
        spacing: 1
        clip: true
        property int currentRow: -1
        model: SitesLogic.tableViewSitesModel

        delegate: Item {
            implicitWidth: 170 * 2
            implicitHeight: 30
            Item {
                width: 170
                height: 30
                anchors.left: parent.left
                id: c1
                Rectangle {
                    anchors.top: parent.top
                    width: parent.width
                    height: 1
                    color: "lightgray"
                    visible: index !== tb.currentRow
                }
                Rectangle {
                    anchors.fill: parent
                    color: "#63B4FB"
                    visible: index === tb.currentRow

                }
                Text {
                    text: url_path
                    anchors.fill: parent
                    leftPadding: 10
                    verticalAlignment: Text.AlignVCenter
                }
            }
            Item {
                anchors.left: c1.right
                width: 170
                height: 30
                Rectangle {
                    anchors.top: parent.top
                    width: parent.width
                    height: 1
                    color: "lightgray"
                    visible: index !== tb.currentRow
                }
                Rectangle {
                    anchors.fill: parent
                    color: "#63B4FB"
                    visible: index === tb.currentRow

                }
                Text {
                    text: ip
                    anchors.fill: parent
                    leftPadding: 10
                    verticalAlignment: Text.AlignVCenter
                }
            }
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    tb.currentRow = index
                }
            }
        }
    }
}
