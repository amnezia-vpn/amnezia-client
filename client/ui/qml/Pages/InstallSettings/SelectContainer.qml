import QtQuick
import QtQuick.Controls
import SortFilterProxyModel 0.2
import ProtocolEnum 1.0
import "./"
import "../../Controls"
import "../../Config"

Drawer {
    id: root
    signal containerSelected(int c_index)
    property int selectedIndex: -1

    y: 0
    x: 0
    edge: Qt.RightEdge
    width: parent.width * 0.85
    height: parent.height

    modal: true
    interactive: activeFocus

    SortFilterProxyModel {
        id: proxyModel
        sourceModel: UiLogic.containersModel
        filters: [
            ValueFilter {
                roleName: "is_installed_role"
                value: false },
            ValueFilter {
                roleName: "service_type_role"
                value: ProtocolEnum.Vpn }
        ]

    }

    SortFilterProxyModel {
        id: proxyModel_other
        sourceModel: UiLogic.containersModel
        filters: [
            ValueFilter {
                roleName: "is_installed_role"
                value: false },
            ValueFilter {
                roleName: "service_type_role"
                value: ProtocolEnum.Other }
        ]

    }

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
                currentIndex: -1
                width: parent.width - 20
                height: contentItem.height

                spacing: 0
                clip: true
                interactive: false
                model: proxyModel

                delegate: Item {
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
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            tb.currentIndex = index
                            tb_other.currentIndex = -1
                            containerSelected(proxyModel.mapToSource(index))
                            selectedIndex = proxyModel.mapToSource(index)
                            root.close()
                        }
                    }
                }
            }


            Caption {
                id: cap2
                font.pixelSize: 20
                text: qsTr("Other containers")
            }

            ListView {
                id: tb_other
                x: 10
                currentIndex: -1
                width: parent.width - 20
                height: contentItem.height

                spacing: 0
                clip: true
                interactive: false
                model: proxyModel_other

                delegate: Item {
                    implicitWidth: 170 * 2
                    implicitHeight: 30
                    Item {
                        width: parent.width
                        height: 30
                        anchors.left: parent.left
                        id: c1_other
                        Rectangle {
                            anchors.top: parent.top
                            width: parent.width
                            height: 1
                            color: "lightgray"
                            visible: index !== tb_other.currentIndex
                        }
                        Rectangle {
                            anchors.fill: parent
                            color: "#63B4FB"
                            visible: index === tb_other.currentIndex

                        }
                        Text {
                            id: text_name_other
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
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            tb_other.currentIndex = index
                            tb.currentIndex = -1
                            containerSelected(proxyModel_other.mapToSource(index))
                            selectedIndex = proxyModel_other.mapToSource(index)
                            root.close()
                        }
                    }
                }
            }


        }


    }

}
