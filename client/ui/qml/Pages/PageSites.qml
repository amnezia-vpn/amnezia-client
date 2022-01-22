import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQml.Models 2.15
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

    property int lastIndex: 0

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

    DelegateModel {
        id: visualModel
        model: SitesLogic.tableViewSitesModel
        groups: [
            DelegateModelGroup {
                id : delegateModelGroup
                name: "multiSelect"
                function removeAll(){
                    var count = delegateModelGroup.count;
                    if (count !== 0){
                        delegateModelGroup.remove(0,count);
                    }
                }
            }
        ]
        delegate: Rectangle {
            id: item
            focus: true
            height: 25
            width: root.width
            color: item.DelegateModel.inMultiSelect ? '#63b4fb' : 'transparent'
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
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                onClicked:{
                    tb.focus = true
                    if(mouse.button === Qt.RightButton){
                        //copyPasteMenu.popup()
                        console.log("RightButton")
                    }
                    if(mouse.button === Qt.LeftButton){
                        switch(mouse.modifiers){
                        case Qt.ControlModifier :
                            item.DelegateModel.inMultiSelect = !item.DelegateModel.inMultiSelect
                            break;
                        case Qt.ShiftModifier :
                            delegateModelGroup.removeAll();
                            var start = lastIndex <= index? lastIndex: index;
                            var end = lastIndex >= index? lastIndex: index;
                            for(var i = start;i <= end;i++){
                                visualModel.items.get(i).inMultiSelect = true
                            }
                            break;
                        default:
                            delegateModelGroup.removeAll();
                            item.DelegateModel.inMultiSelect = true
                            lastIndex = index
                            break;
                        }
                    }
                }
            }
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
        focus: true
        activeFocusOnTab: true
        keyNavigationEnabled: true
        property int currentRow: -1
        //model: SitesLogic.tableViewSitesModel
        model: visualModel

    }

    BlueButtonType {
        id: sites_delete
        anchors.bottom: select_all.top
        anchors.bottomMargin: 10
        anchors.horizontalCenter: parent.horizontalCenter
        height: 31
        font.pixelSize: 16
        text: qsTr("Delete selected")
        onClicked: {
            var items = []
            for(var i = 0; i < visualModel.count; i++){
                if (visualModel.items.get(i).inMultiSelect) items.push(i)
            }

            console.debug(items)
            SitesLogic.onPushButtonSitesDeleteClicked(items)
        }
    }

    BlueButtonType {
        id: select_all
        anchors.bottom: sites_export.top
        anchors.bottomMargin: 10
        anchors.horizontalCenter: parent.horizontalCenter
        height: 31
        font.pixelSize: 16
        text: qsTr("Select all")
        onClicked: {
            for(var i = 0; i < visualModel.count; i++){
                visualModel.items.get(i).inMultiSelect = true
            }
        }
    }

    BlueButtonType {
        id: sites_export
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 20
        anchors.horizontalCenter: parent.horizontalCenter
        height: 31
        font.pixelSize: 16
        text: qsTr("Export all")
        onClicked: {
            SitesLogic.onPushButtonSitesExportClicked()
        }
    }
}
