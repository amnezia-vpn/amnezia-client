import QtQuick 2.12
import QtQuick.Controls 2.12
import Qt.labs.platform 1.0
import QtQuick.Dialogs 1.0
import PageEnum 1.0
import "./"
import "../Controls"
import "../Config"

PageBase {
    id: root
    page: PageEnum.Sites
    logic: SitesLogic

    BackButton {
        id: back
    }

    Caption {
        id: caption
        text: SitesLogic.labelSitesAddCustomText
    }

    LabelType {
        id: lb_addr
        color: "#333333"
        text: qsTr("Web site/Hostname/IP address/Subnet")
        x: 20
        anchors.top: caption.bottom
        anchors.topMargin: 10
        width: parent.width
        height: 21
    }

    TextFieldType {
        anchors.top: lb_addr.bottom
        anchors.topMargin: 10
        anchors.left: parent.left
        anchors.leftMargin: 20
        anchors.right: sites_add.left
        anchors.rightMargin: 10
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
        anchors.right: sites_import.left
        anchors.rightMargin: 10
        anchors.top: lb_addr.bottom
        anchors.topMargin: 10
        width: 51
        height: 31
        font.pixelSize: 24
        text: "+"
        onClicked: {
            SitesLogic.onPushButtonAddCustomSitesClicked()
        }
    }

    BasicButtonType {
        id: sites_import
        anchors.right: parent.right
        anchors.rightMargin: 20
        anchors.top: lb_addr.bottom
        anchors.topMargin: 10
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
        anchors.top: sites_add.bottom
        anchors.topMargin: 10
        width: parent.width - 40
        anchors.bottom: sites_delete.top
        anchors.bottomMargin: 10
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

    BlueButtonType {
        id: sites_delete
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 20
        anchors.horizontalCenter: parent.horizontalCenter
        height: 31
        font.pixelSize: 16
        text: qsTr("Delete selected")
        onClicked: {
            SitesLogic.onPushButtonSitesDeleteClicked(tb.currentRow)
        }
    }
}
