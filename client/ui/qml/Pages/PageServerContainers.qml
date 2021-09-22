import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.15
import SortFilterProxyModel 0.2
import ContainerProps 1.0
import ProtocolProps 1.0
import PageEnum 1.0
import ProtocolEnum 1.0
import "./"
import "../Controls"
import "../Config"
import "InstallSettings"

PageBase {
    id: root
    page: PageEnum.ServerContainers
    logic: ServerContainersLogic

    enabled: ServerContainersLogic.pageEnabled

    function resetPage() {
        container_selector.selectedIndex = -1
    }

    Connections {
        target: logic
        function onUpdatePage() {
            root.resetPage()
        }
    }

    BackButton {
        id: back
    }
    Caption {
        id: caption
        text: container_selector.selectedIndex > 0 ? qsTr("Install new service") : qsTr("Installed services")
    }

    SelectContainer {
        id: container_selector

        onContainerSelected: {
            var containerProto =  ContainerProps.defaultProtocol(c_index)

            tf_port_num.text = ProtocolProps.defaultPort(containerProto)
            cb_port_proto.currentIndex = ProtocolProps.defaultTransportProto(containerProto)

            tf_port_num.enabled = ProtocolProps.defaultPortChangeable(containerProto)
            cb_port_proto.enabled = ProtocolProps.defaultTransportProtoChangeable(containerProto)
        }
    }

    Column {
        id: c1
        visible: container_selector.selectedIndex > 0
        width: parent.width
        anchors.top: caption.bottom
        anchors.topMargin: 10

        Caption {
            font.pixelSize: 22
            text: UiLogic.containerName(container_selector.selectedIndex)
        }

        Text {
            width: parent.width
            anchors.topMargin: 10
            padding: 10

            font.family: "Lato"
            font.styleName: "normal"
            font.pixelSize: 16
            color: "#181922"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.Wrap

            text: UiLogic.containerDesc(container_selector.selectedIndex)
        }
    }

    Rectangle {
        id: frame_settings
        visible: container_selector.selectedIndex > 0
        width: parent.width
        anchors.top: c1.bottom
        anchors.topMargin: 10

        border.width: 1
        border.color: "lightgray"
        anchors.bottomMargin: 5
        anchors.horizontalCenter: parent.horizontalCenter
        radius: 2
        Grid {
            id: grid
            visible: container_selector.selectedIndex > 0
            anchors.fill: parent
            columns: 2
            horizontalItemAlignment: Grid.AlignHCenter
            verticalItemAlignment: Grid.AlignVCenter
            topPadding: 5
            leftPadding: 10
            spacing: 5


            LabelType {
                width: 130
                text: qsTr("Port")
            }
            TextFieldType {
                id: tf_port_num
                width: parent.width - 130 - parent.spacing - parent.leftPadding * 2
            }
            LabelType {
                width: 130
                text: qsTr("Network Protocol")
            }
            ComboBoxType {
                id: cb_port_proto
                width: parent.width - 130 - parent.spacing - parent.leftPadding * 2
                model: [
                    qsTr("udp"),
                    qsTr("tcp"),
                ]
            }
        }
    }

    BlueButtonType {
        id: pb_cancel_add
        visible: container_selector.selectedIndex > 0

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: pb_continue_add.top
        anchors.bottomMargin: 20

        width: parent.width - 40
        height: 40
        text: qsTr("Cancel")
        font.pixelSize: 16
        onClicked: container_selector.selectedIndex = -1

    }

    BlueButtonType {
        id: pb_continue_add
        visible: container_selector.selectedIndex > 0

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 20

        width: parent.width - 40
        height: 40
        text: qsTr("Continue")
        font.pixelSize: 16
        onClicked: {
            let cont = container_selector.selectedIndex
            let tp = ProtocolProps.transportProtoFromString(cb_port_proto.currentText)
            let port = tf_port_num.text
            ServerContainersLogic.onPushButtonContinueClicked(cont, port, tp)
        }
    }





    Flickable {
        visible: container_selector.selectedIndex <= 0
        clip: true
        width: parent.width
        anchors.top: caption.bottom
        anchors.bottom: pb_add_container.top
        contentHeight: col.height

        Column {
            visible: container_selector.selectedIndex <= 0
            id: col
            anchors {
                left: parent.left;
                right: parent.right;
            }
            topPadding: 20
            spacing: 10

            Caption {
                id: cap1
                text: qsTr("Installed Protocols and Services")
                font.pixelSize: 20

            }

            SortFilterProxyModel {
                id: proxyContainersModel
                sourceModel: UiLogic.containersModel
                filters: ValueFilter {
                    roleName: "is_installed_role"
                    value: true
                }
            }

            SortFilterProxyModel {
                id: proxyProtocolsModel
                sourceModel: UiLogic.protocolsModel
                filters: ValueFilter {
                    roleName: "is_installed_role"
                    value: true
                }
            }


            ListView {
                id: tb_c
                x: 10
                width: parent.width - 10
                height: tb_c.contentItem.height
                currentIndex: -1
                spacing: 5
                clip: true
                interactive: false
                model: proxyContainersModel

                delegate: Item {
                    implicitWidth: tb_c.width - 10
                    implicitHeight: c_item.height
                    Item {
                        id: c_item
                        width: parent.width
                        height: row_container.height + tb_p.height
                        anchors.left: parent.left
                        Rectangle {
                            anchors.top: parent.top
                            width: parent.width
                            height: 1
                            color: "lightgray"
                            visible: index !== tb_c.currentIndex
                        }
                        Rectangle {
                            anchors.top: row_container.top
                            anchors.bottom: row_container.bottom
                            anchors.left: parent.left
                            anchors.right: parent.right

                            color: "#63B4FB"
                            visible: index === tb_c.currentIndex
                        }

                        RowLayout {
                            id: row_container
                            //width: parent.width
                            anchors.left: parent.left
                            anchors.right: parent.right

//                            anchors.top: lb_container_name.top
//                            anchors.bottom: lb_container_name.bottom

                            Text {
                                id: lb_container_name
                                text: name_role
                                font.pixelSize: 17
                                //font.bold: true
                                color: "#100A44"
                                topPadding: 5
                                bottomPadding: 5
                                leftPadding: 10
                                verticalAlignment: Text.AlignVCenter
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true

                                MouseArea {
                                    enabled: col.visible
                                    anchors.top: lb_container_name.top
                                    anchors.bottom: lb_container_name.bottom
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    propagateComposedEvents: true
                                    onClicked: {
                                        if (tb_c.currentIndex === index) tb_c.currentIndex = -1
                                        else tb_c.currentIndex = index

                                        UiLogic.protocolsModel.setSelectedDockerContainer(proxyContainersModel.mapToSource(index))
                                    }
                                }
                            }

                            ImageButtonType {
                                id: button_default
                                visible: service_type_role == ProtocolEnum.Vpn

                                Layout.alignment: Qt.AlignRight
                                checkable: true
                                img.source: checked ? "qrc:/images/check.png" : "qrc:/images/uncheck.png"
                                implicitWidth: 30
                                implicitHeight: 30

                                checked: default_role
                                onClicked: {
                                    ServerContainersLogic.onPushButtonDefaultClicked(proxyContainersModel.mapToSource(index))
                                }
                            }

                            ImageButtonType {
                                id: button_share
                                visible: index === tb_c.currentIndex
                                Layout.alignment: Qt.AlignRight
                                icon.source: "qrc:/images/share.png"
                                implicitWidth: 30
                                implicitHeight: 30
                                onClicked: {
                                    ServerContainersLogic.onPushButtonShareClicked(proxyContainersModel.mapToSource(index))
                                }

                                VisibleBehavior on visible { }
                            }

                            ImageButtonType {
                                id: button_remove
                                visible: index === tb_c.currentIndex
                                Layout.alignment: Qt.AlignRight
                                checkable: true
                                icon.source: "qrc:/images/delete.png"
                                implicitWidth: 30
                                implicitHeight: 30

                                checked: default_role
                                onClicked: {
                                    ServerContainersLogic.onPushButtonRemoveClicked(proxyContainersModel.mapToSource(index))
                                }

                                VisibleBehavior on visible { }
                            }

                        }


                        ListView {
                            id: tb_p
                            currentIndex: -1
                            visible: index === tb_c.currentIndex
                            x: 10
                            anchors.top: row_container.bottom

                            width: parent.width - 40
                            height: visible ? tb_p.contentItem.height : 0

                            spacing: 0
                            clip: true
                            interactive: false
                            model: proxyProtocolsModel

                            VisibleBehavior on visible { }


                            delegate: Item {
                                id: dp_item

                                implicitWidth: tb_p.width - 10
                                implicitHeight: p_item.height
                                Item {
                                    id: p_item
                                    width: parent.width
                                    height: lb_protocol_name.height
                                    anchors.left: parent.left
                                    Rectangle {
                                        anchors.top: parent.top
                                        width: parent.width
                                        height: 1
                                        color: "lightgray"
                                        visible: index !== tb_p.currentIndex
                                    }
//                                    Rectangle {
//                                        anchors.top: lb_protocol_name.top
//                                        anchors.bottom: lb_protocol_name.bottom
//                                        width: parent.width

//                                        color: "#63B4FB"
//                                        visible: index === tb_p.currentIndex
//                                    }

//                                    Text {
//                                        id: lb_protocol_name
//                                        text: name_role
//                                        font.pixelSize: 16
//                                        topPadding: 5
//                                        bottomPadding: 5
//                                        leftPadding: 10
//                                        verticalAlignment: Text.AlignVCenter
//                                        wrapMode: Text.WordWrap
//                                    }

                                    SettingButtonType {
                                        id: lb_protocol_name

//                                        anchors.top: lb_protocol_name.top
//                                        anchors.bottom: lb_protocol_name.bottom
                                        topPadding: 10
                                        bottomPadding: 10
                                        leftPadding: 10

                                        anchors.left: parent.left

                                        width: parent.width
                                        height: 30
                                        text: qsTr(name_role + " settings")
                                        textItem.font.pixelSize: 16
                                        icon.source: "qrc:/images/settings.png"
                                        onClicked: {
                                            tb_p.currentIndex = index
                                            ServerContainersLogic.onPushButtonProtoSettingsClicked(
                                                        proxyContainersModel.mapToSource(tb_c.currentIndex),
                                                        proxyProtocolsModel.mapToSource(tb_p.currentIndex))
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }


    BlueButtonType {
        id: pb_add_container
        visible: container_selector.selectedIndex < 0

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.topMargin: 10
        anchors.bottomMargin: 20

        width: parent.width - 40
        height: 40
        text: qsTr("Install new protocols container")
        font.pixelSize: 16
        onClicked: container_selector.visible ? container_selector.close() : container_selector.open()

    }
}
