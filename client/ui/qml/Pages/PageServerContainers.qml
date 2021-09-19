import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.15
import SortFilterProxyModel 0.2
import PageEnum 1.0
import "./"
import "../Controls"
import "../Config"
import "InstallSettings"

PageBase {
    id: root
    page: PageEnum.ServerContainers
    logic: ServerContainersLogic

    enabled: ServerContainersLogic.pageEnabled
    BackButton {
        id: back
    }
    Caption {
        id: caption
        text: qsTr("Protocols")
    }
    BlueButtonType {
        id: pb_add_container
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: caption.bottom
        anchors.topMargin: 10

        width: parent.width - 40
        height: 40
        text: qsTr("Add protocols container")
        font.pixelSize: 16
        onClicked: container_selector.visible ? container_selector.close() : container_selector.open()

    }
    SelectContainer {
        id: container_selector
    }

    Flickable {
        clip: true
        width: parent.width
        anchors.top: pb_add_container.bottom
        anchors.bottom: parent.bottom
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
                text: qsTr("Installed VPN containers")
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

//                        ImageButtonType {
//                            id: button_default1
//                            z:10

//                            Layout.alignment: Qt.AlignRight
//                            checkable: true
//                            img.source: checked ? "qrc:/images/check.png" : "qrc:/images/uncheck.png"
//                            width: 20
//                            img.width: 20
//                            height: 20

//                            checked: default_role
//                            onClicked: {
//                                ServerContainersLogic.onPushButtonDefaultClicked(proxyContainersModel.mapToSource(index))
//                            }
//                        }

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
                                    anchors.top: lb_container_name.top
                                    anchors.bottom: lb_container_name.bottom
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    propagateComposedEvents: true
                                    onClicked: {
                                        if (tb_c.currentIndex === index) tb_c.currentIndex = -1
                                        else tb_c.currentIndex = index

                                        UiLogic.protocolsModel.setSelectedDockerContainer(proxyContainersModel.mapToSource(index))
                                        //ServerContainersLogic.setSelectedDockerContainer(proxyContainersModel.mapToSource(index))

                                        //container_selector.containerSelected(index)
                                        //root.close()
                                    }
                                }
                            }

                            ImageButtonType {
                                id: button_default

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
                                Layout.alignment: Qt.AlignRight
                                icon.source: "qrc:/images/share.png"
                                implicitWidth: 30
                                implicitHeight: 30
                                onClicked: {
                                    ServerContainersLogic.onPushButtonShareClicked(proxyContainersModel.mapToSource(index))
                                }
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

//                                MouseArea {
//                                    anchors.fill: parent
//                                    onClicked: {
//                                        tb_p.currentIndex = index
//                                    }
//                                }
                            }
                        }
                    }
                }
            }
        }


    }











//    ProgressBar {
//        id: progress_bar
//        anchors.horizontalCenter: parent.horizontalCenter
//        y: 570
//        width: 301
//        height: 40
//        from: 0
//        to: ServerContainersLogic.progressBarProtocolsContainerReinstallMaximium
//        value: ServerContainersLogic.progressBarProtocolsContainerReinstallValue
//        visible: ServerContainersLogic.progressBarProtocolsContainerReinstallVisible
//        background: Rectangle {
//            implicitWidth: parent.width
//            implicitHeight: parent.height
//            color: "#100A44"
//            radius: 4
//        }

//        contentItem: Item {
//            implicitWidth: parent.width
//            implicitHeight: parent.height
//            Rectangle {
//                width: progress_bar.visualPosition * parent.width
//                height: parent.height
//                radius: 4
//                color: Qt.rgba(255, 255, 255, 0.15);
//            }
//        }

//        LabelType {
//            anchors.fill: parent
//            text: qsTr("Configuring...")
//            horizontalAlignment: Text.AlignHCenter
//            font.family: "Lato"
//            font.styleName: "normal"
//            font.pixelSize: 16
//            color: "#D4D4D4"
//        }
//    }
//    ScrollView {
//        x: 0
//        y: 190
//        width: 380
//        height: 471
//        clip: true
//        Column {
//            spacing: 5
//            Rectangle {
//                id: frame_openvpn_ss_cloak
//                x: 9
//                height: 135
//                width: 363
//                border.width: 1
//                border.color: "lightgray"
//                radius: 2
//                visible: ServerContainersLogic.frameOpenvpnSsCloakSettingsVisible
//                Item {
//                    x: 5
//                    y: 5
//                    width: parent.width - 10
//                    height: parent.height - 10
//                    LabelType {
//                        anchors.left: parent.left
//                        width: 239
//                        height: 24
//                        text: qsTr("Cloak container")
//                        leftPadding: 5
//                    }
//                    ImageButtonType {
//                        anchors.right: sr1.left
//                        anchors.rightMargin: 5
//                        checkable: true
//                        icon.source: checked ? "qrc:/images/check.png" : "qrc:/images/uncheck.png"
//                        width: 24
//                        height: 24
//                        checked: ServerContainersLogic.pushButtonCloakOpenVpnContDefaultChecked
//                        onCheckedChanged: {
//                            ServerContainersLogic.pushButtonCloakOpenVpnContDefaultChecked = checked
//                        }
//                        onClicked: {
//                            ServerContainersLogic.onPushButtonProtoCloakOpenVpnContDefaultClicked(checked)
//                        }

//                        visible: ServerContainersLogic.pushButtonCloakOpenVpnContDefaultVisible
//                    }

//                    ImageButtonType {
//                        id: sr1
//                        anchors.right: cn1.left
//                        anchors.rightMargin: 5
//                        icon.source: "qrc:/images/share.png"
//                        width: 24
//                        height: 24
//                        visible: ServerContainersLogic.pushButtonCloakOpenVpnContShareVisible
//                        onClicked: {
//                            ServerContainersLogic.onPushButtonProtoCloakOpenVpnContShareClicked(false)
//                        }
//                    }
//                    ImageButtonType {
//                        id: cn1
//                        anchors.right: parent.right
//                        checkable: true
//                        icon.source: checked ? "qrc:/images/connect_button_connected.png"
//                                             : "qrc:/images/connect_button_disconnected.png"
//                        width: 36
//                        height: 24
//                        checked: ServerContainersLogic.pushButtonCloakOpenVpnContInstallChecked
//                        onCheckedChanged: {
//                            ServerContainersLogic.pushButtonCloakOpenVpnContInstallChecked = checked
//                        }
//                        onClicked: {
//                            ServerContainersLogic.onPushButtonProtoCloakOpenVpnContInstallClicked(checked)
//                        }
//                        enabled: ServerContainersLogic.pushButtonCloakOpenVpnContInstallEnabled
//                    }
//                }
//                Rectangle {
//                    x: 10
//                    y: 42
//                    height: 83
//                    width: 343
//                    border.width: 1
//                    border.color: "lightgray"
//                    radius: 2
//                    SettingButtonType {
//                        x: 10
//                        y: 10
//                        width: 323
//                        height: 24
//                        text: qsTr("OpenVPN settings")
//                        icon.source: "qrc:/images/settings.png"
//                        onClicked: {
//                            ServerContainersLogic.onPushButtonProtoCloakOpenVpnContOpenvpnConfigClicked()
//                        }
//                    }
//                    SettingButtonType {
//                        x: 10
//                        y: 33
//                        width: 323
//                        height: 24
//                        text: qsTr("ShadowSocks settings")
//                        icon.source: "qrc:/images/settings.png"
//                        onClicked: {
//                            ServerContainersLogic.onPushButtonProtoCloakOpenVpnContSsConfigClicked()
//                        }
//                    }
//                    SettingButtonType {
//                        x: 10
//                        y: 56
//                        width: 323
//                        height: 24
//                        text: qsTr("Cloak settings")
//                        icon.source: "qrc:/images/settings.png"
//                        onClicked: {
//                            ServerContainersLogic.onPushButtonProtoCloakOpenVpnContCloakConfigClicked()
//                        }
//                    }
//                }
//            }
//            Rectangle {
//                id: frame_openvpn_ss
//                x: 9
//                height: 105
//                width: 363
//                border.width: 1
//                border.color: "lightgray"
//                radius: 2
//                visible: ServerContainersLogic.frameOpenvpnSsSettingsVisible
//                Item {
//                    x: 5
//                    y: 5
//                    width: parent.width - 10
//                    height: parent.height - 10
//                    LabelType {
//                        anchors.left: parent.left
//                        width: 239
//                        height: 24
//                        text: qsTr("ShadowSocks container")
//                        leftPadding: 5
//                    }
//                    ImageButtonType {
//                        anchors.right: sr2.left
//                        anchors.rightMargin: 5
//                        checkable: true
//                        icon.source: checked ? "qrc:/images/check.png" : "qrc:/images/uncheck.png"
//                        width: 24
//                        height: 24
//                        checked: ServerContainersLogic.pushButtonSsOpenVpnContDefaultChecked
//                        onCheckedChanged: {
//                            ServerContainersLogic.pushButtonSsOpenVpnContDefaultChecked = checked
//                        }
//                        onClicked: {
//                            ServerContainersLogic.onPushButtonProtoSsOpenVpnContDefaultClicked(checked)
//                        }

//                        visible: ServerContainersLogic.pushButtonSsOpenVpnContDefaultVisible
//                    }

//                    ImageButtonType {
//                        id: sr2
//                        anchors.right: cn2.left
//                        anchors.rightMargin: 5
//                        icon.source: "qrc:/images/share.png"
//                        width: 24
//                        height: 24
//                        visible: ServerContainersLogic.pushButtonSsOpenVpnContShareVisible
//                        onClicked: {
//                            ServerContainersLogic.onPushButtonProtoSsOpenVpnContShareClicked(false)
//                        }
//                    }
//                    ImageButtonType {
//                        id: cn2
//                        anchors.right: parent.right
//                        checkable: true
//                        icon.source: checked ? "qrc:/images/connect_button_connected.png"
//                                             : "qrc:/images/connect_button_disconnected.png"
//                        width: 36
//                        height: 24
//                        checked: ServerContainersLogic.pushButtonSsOpenVpnContInstallChecked
//                        onCheckedChanged: {
//                            ServerContainersLogic.pushButtonSsOpenVpnContInstallChecked = checked
//                        }
//                        onClicked: {
//                            ServerContainersLogic.onPushButtonProtoSsOpenVpnContInstallClicked(checked)
//                        }
//                        enabled: ServerContainersLogic.pushButtonSsOpenVpnContInstallEnabled
//                    }
//                }
//                Rectangle {
//                    x: 10
//                    y: 42
//                    height: 53
//                    width: 343
//                    border.width: 1
//                    border.color: "lightgray"
//                    radius: 2
//                    SettingButtonType {
//                        x: 10
//                        y: 5
//                        width: 323
//                        height: 24
//                        text: qsTr("OpenVPN settings")
//                        icon.source: "qrc:/images/settings.png"
//                        onClicked: {
//                            ServerContainersLogic.onPushButtonProtoSsOpenVpnContOpenvpnConfigClicked()
//                        }
//                    }
//                    SettingButtonType {
//                        x: 10
//                        y: 27
//                        width: 323
//                        height: 24
//                        text: qsTr("ShadowSocks settings")
//                        icon.source: "qrc:/images/settings.png"
//                        onClicked: {
//                            ServerContainersLogic.onPushButtonProtoSsOpenVpnContSsConfigClicked()
//                        }
//                    }
//                }
//            }
//            Rectangle {
//                id: frame_openvpn
//                x: 9
//                height: 100
//                width: 363
//                border.width: 1
//                border.color: "lightgray"
//                radius: 2
//                visible: ServerContainersLogic.frameOpenvpnSettingsVisible
//                Item {
//                    x: 5
//                    y: 5
//                    width: parent.width - 10
//                    height: parent.height - 10
//                    LabelType {
//                        anchors.left: parent.left
//                        width: 239
//                        height: 24
//                        text: qsTr("OpenVPN container")
//                        leftPadding: 5
//                    }
//                    ImageButtonType {
//                        anchors.right: sr3.left
//                        anchors.rightMargin: 5
//                        checkable: true
//                        icon.source: checked ? "qrc:/images/check.png" : "qrc:/images/uncheck.png"
//                        width: 24
//                        height: 24
//                        checked: ServerContainersLogic.pushButtonOpenVpnContDefaultChecked
//                        onCheckedChanged: {
//                            ServerContainersLogic.pushButtonOpenVpnContDefaultChecked = checked
//                        }
//                        onClicked: {
//                            ServerContainersLogic.onPushButtonProtoOpenVpnContDefaultClicked(checked)
//                        }

//                        visible: ServerContainersLogic.pushButtonOpenVpnContDefaultVisible
//                    }

//                    ImageButtonType {
//                        id: sr3
//                        anchors.right: cn3.left
//                        anchors.rightMargin: 5
//                        icon.source: "qrc:/images/share.png"
//                        width: 24
//                        height: 24
//                        visible: ServerContainersLogic.pushButtonOpenVpnContShareVisible
//                        onClicked: {
//                            ServerContainersLogic.onPushButtonProtoOpenVpnContShareClicked(false)
//                        }
//                    }
//                    ImageButtonType {
//                        id: cn3
//                        anchors.right: parent.right
//                        checkable: true
//                        icon.source: checked ? "qrc:/images/connect_button_connected.png"
//                                             : "qrc:/images/connect_button_disconnected.png"
//                        width: 36
//                        height: 24
//                        checked: ServerContainersLogic.pushButtonOpenVpnContInstallChecked
//                        onCheckedChanged: {
//                            ServerContainersLogic.pushButtonOpenVpnContInstallChecked = checked
//                        }
//                        onClicked: {
//                            ServerContainersLogic.onPushButtonProtoOpenVpnContInstallClicked(checked)
//                        }
//                        enabled: ServerContainersLogic.pushButtonOpenVpnContInstallEnabled
//                    }
//                }
//                Rectangle {
//                    x: 10
//                    y: 42
//                    height: 44
//                    width: 343
//                    border.width: 1
//                    border.color: "lightgray"
//                    radius: 2
//                    SettingButtonType {
//                        x: 10
//                        y: 10
//                        width: 323
//                        height: 24
//                        text: qsTr("OpenVPN settings")
//                        icon.source: "qrc:/images/settings.png"
//                        onClicked: {
//                            ServerContainersLogic.onPushButtonProtoOpenVpnContOpenvpnConfigClicked()
//                        }
//                    }
//                }
//            }
//            Rectangle {
//                id: frame_wireguard
//                x: 9
//                height: 100
//                width: 363
//                border.width: 1
//                border.color: "lightgray"
//                radius: 2
//                visible: ServerContainersLogic.frameWireguardVisible
//                Item {
//                    x: 5
//                    y: 5
//                    width: parent.width - 10
//                    height: parent.height - 10
//                    LabelType {
//                        anchors.left: parent.left
//                        width: 239
//                        height: 24
//                        text: qsTr("WireGuard container")
//                        leftPadding: 5
//                    }
//                    ImageButtonType {
//                        anchors.right: sr4.left
//                        anchors.rightMargin: 5
//                        checkable: true
//                        icon.source: checked ? "qrc:/images/check.png" : "qrc:/images/uncheck.png"
//                        width: 24
//                        height: 24
//                        checked: ServerContainersLogic.pushButtonWireguardContDefaultChecked
//                        onCheckedChanged: {
//                            ServerContainersLogic.pushButtonWireguardContDefaultChecked = checked
//                        }
//                        onClicked: {
//                            ServerContainersLogic.onPushButtonProtoWireguardContDefaultClicked(checked)
//                        }

//                        visible: ServerContainersLogic.pushButtonWireguardContDefaultVisible
//                    }

//                    ImageButtonType {
//                        id: sr4
//                        anchors.right: cn4.left
//                        anchors.rightMargin: 5
//                        icon.source: "qrc:/images/share.png"
//                        width: 24
//                        height: 24
//                        visible: ServerContainersLogic.pushButtonWireguardContShareVisible
//                        onClicked: {
//                            ServerContainersLogic.onPushButtonProtoWireguardContShareClicked(false)
//                        }
//                    }
//                    ImageButtonType {
//                        id: cn4
//                        anchors.right: parent.right
//                        checkable: true
//                        icon.source: checked ? "qrc:/images/connect_button_connected.png"
//                                             : "qrc:/images/connect_button_disconnected.png"
//                        width: 36
//                        height: 24
//                        checked: ServerContainersLogic.pushButtonWireguardContInstallChecked
//                        onCheckedChanged: {
//                            ServerContainersLogic.pushButtonWireguardContInstallChecked = checked
//                        }
//                        onClicked: {
//                            ServerContainersLogic.onPushButtonProtoWireguardContInstallClicked(checked)
//                        }
//                        enabled: ServerContainersLogic.pushButtonWireguardContInstallEnabled
//                    }
//                }
//                Rectangle {
//                    id: frame_wireguard_settings
//                    visible: ServerContainersLogic.frameWireguardSettingsVisible
//                    x: 10
//                    y: 42
//                    height: 44
//                    width: 343
//                    border.width: 1
//                    border.color: "lightgray"
//                    radius: 2
//                    SettingButtonType {
//                        x: 10
//                        y: 10
//                        width: 323
//                        height: 24
//                        text: qsTr("WireGuard settings")
//                        icon.source: "qrc:/images/settings.png"
//                    }
//                }
//            }
//        }
//    }
}
