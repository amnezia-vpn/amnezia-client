import QtQuick 2.12
import QtQuick.Controls 2.12
import "./"
import "../../Controls"
import "../../Config"

Drawer {
    id: root
    signal containerSelected(int id)
    property alias selectedIndex: tb.currentIndex
    property var filter: function (item){ return item.is_vpn_role }

    z: -3

    y: 0
    x: 0
    edge: Qt.RightEdge
    width: parent.width * 0.85
    height: parent.height

    modal: true
    interactive: true

    Flickable {
        clip: true
        anchors.fill: parent
        contentHeight: col.height

        Column {
            id: col
            anchors {
                left: parent.left;
                right: parent.right;
            }
            topPadding: 20
            spacing: 10

            Caption {
                id: cap1
                text: qsTr("VPN containers")
                font.pixelSize: 20

            }

            ListView {
                id: tb
                x: 10
                width: parent.width - 40
                height: contentItem.height

                spacing: 1
                clip: true
                interactive: false
                model: UiLogic.containersModel

                delegate: Item {
                    required property int index

                    required property string name_role
                    required property string desc_role
                    required property bool is_vpn_role
                    required property bool is_other_role
                    required property bool is_installed_role

                    visible: filter(this)
                    implicitWidth: 170 * 2
                    implicitHeight: 30
                    Item {
                        width: parent.width
                        height: 30
                        anchors.left: parent.left
                        id: c1
                        Rectangle {
                            anchors.top: parent.top
                            width: parent.width
                            height: 1
                            color: "lightgray"
                            visible: index !== tb.currentIndex
                        }
                        Rectangle {
                            anchors.fill: parent
                            color: "#63B4FB"
                            visible: index === tb.currentIndex

                        }
                        Text {
                            id: text_name
                            text: name_role
                            font.pixelSize: 16
                            anchors.fill: parent
                            leftPadding: 10
                            verticalAlignment: Text.AlignVCenter
                            wrapMode: Text.WordWrap
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            tb.currentIndex = index
                            containerSelected(index)
                            root.close()
                        }
                    }
                }
            }


//            Caption {
//                id: cap2
//                text: qsTr("Other containers")
//            }

//            ListView {
//                id: tb_other
//                x: 10
//                //y: 20
//                width: parent.width - 40
//                height: contentItem.height

//                spacing: 1
//                clip: true
//                interactive: false
//                property int currentRow: -1
//                model: UiLogic.containersModel

//                delegate: Item {
//                    implicitWidth: 170 * 2
//                    implicitHeight: 30
//                    Item {
//                        width: parent.width
//                        height: 30
//                        anchors.left: parent.left
//                        id: c1_other
//                        Rectangle {
//                            anchors.top: parent.top
//                            width: parent.width
//                            height: 1
//                            color: "lightgray"
//                            visible: index !== tb_other.currentRow
//                        }
//                        Rectangle {
//                            anchors.fill: parent
//                            color: "#63B4FB"
//                            visible: index === tb_other.currentRow

//                        }
//                        Text {
//                            id: text_name_other
//                            text: name
//                            font.pixelSize: 16
//                            anchors.fill: parent
//                            leftPadding: 10
//                            verticalAlignment: Text.AlignVCenter
//                            wrapMode: Text.WordWrap
//                        }
//                    }

//                    MouseArea {
//                        anchors.fill: parent
//                        onClicked: {
//                            tb_other.currentRow = index
//                        }
//                    }
//                }
//            }


        }


    }

}
