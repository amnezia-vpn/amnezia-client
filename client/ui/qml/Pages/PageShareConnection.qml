import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Dialogs 1.1
import QtQuick.Layouts 1.15
import QtGraphicalEffects 1.12
import SortFilterProxyModel 0.2
import ContainerProps 1.0
import ProtocolProps 1.0
import PageEnum 1.0
import ProtocolEnum 1.0
import "./"
import "../Controls"
import "../Config"

PageBase {
    id: root
    page: PageEnum.ShareConnection
    logic: ShareConnectionLogic

    BackButton {
        id: back
    }

    Caption {
        id: caption
        text: qsTr("Share protocol config")
        width: undefined
    }


    Flickable {
        clip: true
        width: parent.width
        anchors.top: caption.bottom
        anchors.bottom: root.bottom
        contentHeight: col.height

        Column {
            id: col
            anchors {
                left: parent.left;
                right: parent.right;
            }
            topPadding: 20
            spacing: 10

//            Caption {
//                id: cap1
//                text: qsTr("Installed Protocols and Services")
//                font.pixelSize: 20

//            }

            SortFilterProxyModel {
                id: proxyProtocolsModel
                sourceModel: UiLogic.protocolsModel
                filters: ValueFilter {
                    roleName: "is_installed_role"
                    value: true
                }
            }


            ShareConnectionContent {
                x: 10
                text: qsTr("Share for Amnezia")
                height: 40
                width: tb_c.width - 10
                onClicked: UiLogic.onGotoShareProtocolPage(ProtocolEnum.Any)
            }

            ListView {
                id: tb_c
                x: 10
                width: parent.width - 10
                height: tb_c.contentItem.height
                currentIndex: -1
                spacing: 10
                clip: true
                interactive: false
                model: proxyProtocolsModel

                delegate: Item {
                    implicitWidth: tb_c.width - 10
                    implicitHeight: c_item.height

                    ShareConnectionContent {
                        id: c_item
                        text: qsTr("Share for ") + name_role
                        height: 40
                        width: tb_c.width - 10
                        onClicked: UiLogic.onGotoShareProtocolPage(proxyProtocolsModel.mapToSource(index))

                    }

//                    Rectangle {
//                        id: c_item
//                        x: 0
//                        y: 0
//                        width: parent.width
//                        height: 40
//                        color: "transparent"
//                        clip: true
//                        radius: 2
//                        LinearGradient {
//                            anchors.fill: parent
//                            start: Qt.point(0, 0)
//                            end: Qt.point(0, height)
//                            gradient: Gradient {
//                                GradientStop { position: 0.0; color: "#E1E1E1" }
//                                GradientStop { position: 0.4; color: "#DDDDDD" }
//                                GradientStop { position: 0.5; color: "#D8D8D8" }
//                                GradientStop { position: 1.0; color: "#D3D3D3" }
//                            }
//                        }
//                        Image {
//                            anchors.verticalCenter: parent.verticalCenter
//                            anchors.left: parent.left
//                            anchors.leftMargin: 10
//                            source: "qrc:/images/share.png"
//                        }
//                        Rectangle {
//                            anchors.left: parent.left
//                            anchors.right: parent.right
//                            anchors.bottom: parent.bottom
//                            height: 2
//                            color: "#148CD2"
//                            visible: ms.containsMouse ? true : false
//                        }
//                        Text {
//                            x: 40
//                            anchors.verticalCenter: parent.verticalCenter
//                            font.family: "Lato"
//                            font.styleName: "normal"
//                            font.pixelSize: 18
//                            color: "#100A44"
//                            font.bold: true
//                            text: name_role
//                            horizontalAlignment: Text.AlignLeft
//                            verticalAlignment: Text.AlignVCenter
//                            wrapMode: Text.Wrap
//                        }
//                        MouseArea {
//                            id: ms
//                            anchors.fill: parent
//                            hoverEnabled: true
//                            onClicked: UiLogic.onGotoShareProtocolPage(proxyProtocolsModel.mapToSource(index))
//                        }
//                    }












//                    Item {
//                        id: c_item
//                        width: parent.width
//                        height: row_container.height
//                        anchors.left: parent.left
//                        Rectangle {
//                            anchors.top: parent.top
//                            width: parent.width
//                            height: 1
//                            color: "lightgray"
//                            visible: index !== tb_c.currentIndex
//                        }
//                        Rectangle {
//                            anchors.top: row_container.top
//                            anchors.bottom: row_container.bottom
//                            anchors.left: parent.left
//                            anchors.right: parent.right

//                            color: "#63B4FB"
//                            visible: index === tb_c.currentIndex
//                        }

//                        RowLayout {
//                            id: row_container
//                            //width: parent.width
//                            anchors.left: parent.left
//                            anchors.right: parent.right

////                            anchors.top: lb_container_name.top
////                            anchors.bottom: lb_container_name.bottom

//                            Text {
//                                id: lb_container_name
//                                text: name_role
//                                font.pixelSize: 17
//                                //font.bold: true
//                                color: "#100A44"
//                                topPadding: 5
//                                bottomPadding: 5
//                                leftPadding: 10
//                                verticalAlignment: Text.AlignVCenter
//                                wrapMode: Text.WordWrap
//                                Layout.fillWidth: true

//                                MouseArea {
//                                    enabled: col.visible
//                                    anchors.top: lb_container_name.top
//                                    anchors.bottom: lb_container_name.bottom
//                                    anchors.left: parent.left
//                                    anchors.right: parent.right
//                                    propagateComposedEvents: true
//                                    onClicked: {
//                                        if (tb_c.currentIndex === index) tb_c.currentIndex = -1
//                                        else tb_c.currentIndex = index

//                                        UiLogic.protocolsModel.setSelectedDockerContainer(proxyContainersModel.mapToSource(index))
//                                    }
//                                }
//                            }
//                        }

//                    }
                }
            }
        }
    }
















//    ScrollView {
//        x: 10
//        y: 40
//        width: 360
//        height: 580
//        Item {
//            id: ct
//            width: parent.width
//            height: childrenRect.height + 10
//            property var contentList: [
//                full_access,
//                share_amezia,
//                share_openvpn,
//                share_shadowshock,
//                share_cloak
//            ]
//            property int currentIndex: ShareConnectionLogic.toolBoxShareConnectionCurrentIndex
//            onCurrentIndexChanged: {
//                ShareConnectionLogic.toolBoxShareConnectionCurrentIndex = currentIndex
//                for (let i = 0; i < contentList.length; ++i) {
//                    if (i == currentIndex) {
//                        contentList[i].active = true
//                    } else {
//                        contentList[i].active = false
//                    }
//                }
//            }

//            function clearActive() {
//                for (let i = 0; i < contentList.length; ++i) {
//                    contentList[i].active = false
//                }
//                currentIndex = -1;
//            }
//            Column {
//                spacing: 5
//                ShareConnectionContent {
//                    id: full_access
//                    x: 0
//                    text: qsTr("Full access")
//                    visible: ShareConnectionLogic.pageShareFullAccessVisible
//                    content: Component {
//                        Item {
//                            width: 360
//                            height: 380
//                            Text {
//                                x: 10
//                                y: 250
//                                width: 341
//                                height: 111
//                                font.family: "Lato"
//                                font.styleName: "normal"
//                                font.pixelSize: 16
//                                color: "#181922"
//                                horizontalAlignment: Text.AlignLeft
//                                verticalAlignment: Text.AlignVCenter
//                                wrapMode: Text.Wrap
//                                text: qsTr("Anyone who logs in with this code will have the same permissions to use VPN and your server as you. \nThis code includes your server credentials!\nProvide this code only to TRUSTED users.")
//                            }
//                            ShareConnectionButtonType {
//                                x: 10
//                                y: 130
//                                width: 341
//                                height: 40
//                                text: ShareConnectionLogic.pushButtonShareFullCopyText
//                                onClicked: {
//                                    ShareConnectionLogic.onPushButtonShareFullCopyClicked()
//                                }
//                            }
//                            ShareConnectionButtonType {
//                                x: 10
//                                y: 180
//                                width: 341
//                                height: 40
//                                text: qsTr("Save file")
//                                onClicked: {
//                                    ShareConnectionLogic.onPushButtonShareFullSaveClicked()
//                                }
//                            }
//                            TextFieldType {
//                                x: 10
//                                y: 10
//                                width: 341
//                                height: 100
//                                verticalAlignment: Text.AlignTop
//                                text: ShareConnectionLogic.textEditShareFullCodeText
//                                onEditingFinished: {
//                                    ShareConnectionLogic.textEditShareFullCodeText = text
//                                }
//                            }
//                        }
//                    }
//                    onClicked: {
//                        if (active) {
//                            ct.currentIndex = -1
//                        } else {
//                            ct.clearActive()
//                            ct.currentIndex = 0
//                        }
//                    }
//                }
//                ShareConnectionContent {
//                    id: share_amezia
//                    x: 0
//                    text: qsTr("Share for Amnezia client")
//                    visible: ShareConnectionLogic.pageShareAmneziaVisible
//                    content: Component {
//                        Item {
//                            width: 360
//                            height: 380
//                        }
//                    }
//                    onClicked: {
//                        if (active) {
//                            ct.currentIndex = -1
//                        } else {
//                            ct.clearActive()
//                            ct.currentIndex = 1
//                        }
//                    }
//                }

//                ShareConnectionContent {
//                    id: share_shadowshock
//                    x: 0
//                    text: qsTr("Share for ShadowSocks client")
//                    visible: ShareConnectionLogic.pageShareShadowSocksVisible
//                    content: Component {
//                        Item {
//                            width: 360
//                            height: 380

//                        }
//                    }
//                    onClicked: {
//                        if (active) {
//                            ct.currentIndex = -1
//                        } else {
//                            ct.clearActive()
//                            ct.currentIndex = 3
//                        }
//                    }
//                }
//                ShareConnectionContent {
//                    id: share_cloak
//                    x: 0
//                    text: qsTr("Share for Cloak client")
//                    visible: ShareConnectionLogic.pageShareCloakVisible
//                    content: Component {
//                        Item {
//                            width: 360
//                            height: 380

//                        }
//                    }
//                    onClicked: {
//                        if (active) {
//                            ct.currentIndex = -1
//                        } else {
//                            ct.clearActive()
//                            ct.currentIndex = 4
//                        }
//                    }
//                }
//            }
//        }
//    }



}
