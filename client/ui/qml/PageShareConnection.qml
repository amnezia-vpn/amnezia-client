import QtQuick 2.12
import QtQuick.Controls 2.12
import "./"

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

    ScrollView {
        x: 10
        y: 40
        width: 360
        height: 580
        Item {
            id: ct
            width: parent.width
            height: childrenRect.height + 10
            property var contentList: [
                full_access,
                share_amezia,
                share_openvpn,
                share_shadowshock,
                share_cloak
            ]
            function clearActive() {
                for (let i = 0; i < contentList.length; ++i) {
                    contentList[i].active = false
                }
            }
            Column {
                spacing: 5
                ShareConnectionContent {
                    id: full_access
                    x: 0
                    text: qsTr("Full access")
                    content: Component {
                        Item {
                            width: 360
                            height: 380
                            Text {
                                x: 10
                                y: 250
                                width: 341
                                height: 111
                                font.family: "Lato"
                                font.styleName: "normal"
                                font.pixelSize: 16
                                color: "#181922"
                                horizontalAlignment: Text.AlignLeft
                                verticalAlignment: Text.AlignVCenter
                                wrapMode: Text.Wrap
                                text: qsTr("Anyone who logs in with this code will have the same permissions to use VPN and your server as you. \nThis code includes your server credentials!\nProvide this code only to TRUSTED users.")
                            }
                            ShareConnectionButtonType {
                                x: 10
                                y: 130
                                width: 341
                                height: 40
                                text: qsTr("Copy")
                            }
                            ShareConnectionButtonType {
                                x: 10
                                y: 180
                                width: 341
                                height: 40
                                text: qsTr("Save file")
                            }
                            TextFieldType {
                                x: 10
                                y: 10
                                width: 341
                                height: 100
                                verticalAlignment: Text.AlignTop
                            }
                        }
                    }
                    onClicked: {
                        if (active) {
                            active = false
                        } else {
                            ct.clearActive()
                            active = true
                        }
                    }
                }
                ShareConnectionContent {
                    id: share_amezia
                    x: 0
                    text: qsTr("Share for Amnezia client")
                    content: Component {
                        Item {
                            width: 360
                            height: 380
                            Text {
                                x: 10
                                y: 280
                                width: 341
                                height: 111
                                font.family: "Lato"
                                font.styleName: "normal"
                                font.pixelSize: 16
                                color: "#181922"
                                horizontalAlignment: Text.AlignLeft
                                verticalAlignment: Text.AlignVCenter
                                wrapMode: Text.Wrap
                                text: qsTr("Anyone who logs in with this code will be able to connect to this VPN server. \nThis code does not include server credentials.")
                            }
                            ShareConnectionButtonType {
                                x: 10
                                y: 180
                                width: 341
                                height: 40
                                text: qsTr("Copy")
                            }
                            ShareConnectionButtonType {
                                x: 10
                                y: 130
                                width: 341
                                height: 40
                                text: qsTr("Generate config")
                            }
                            ShareConnectionButtonType {
                                x: 10
                                y: 230
                                width: 341
                                height: 40
                                text: qsTr("Save file")
                            }
                            TextFieldType {
                                x: 10
                                y: 10
                                width: 341
                                height: 100
                                verticalAlignment: Text.AlignTop
                            }
                        }
                    }
                    onClicked: {
                        if (active) {
                            active = false
                        } else {
                            ct.clearActive()
                            active = true
                        }
                    }
                }
                ShareConnectionContent {
                    id: share_openvpn
                    x: 0
                    text: qsTr("Share for OpenVPN client")
                    content: Component {
                        Item {
                            width: 360
                            height: 380
                            ShareConnectionButtonType {
                                x: 10
                                y: 180
                                width: 341
                                height: 40
                                text: qsTr("Copy")
                            }
                            ShareConnectionButtonType {
                                x: 10
                                y: 130
                                width: 341
                                height: 40
                                text: qsTr("Generate config")
                            }
                            ShareConnectionButtonType {
                                x: 10
                                y: 230
                                width: 341
                                height: 40
                                text: qsTr("Save file")
                            }
                            TextFieldType {
                                x: 10
                                y: 10
                                width: 341
                                height: 100
                                verticalAlignment: Text.AlignTop
                            }
                        }
                    }
                    onClicked: {
                        if (active) {
                            active = false
                        } else {
                            ct.clearActive()
                            active = true
                        }
                    }
                }
                ShareConnectionContent {
                    id: share_shadowshock
                    x: 0
                    text: qsTr("Share for ShadowSocks client")
                    content: Component {
                        Item {
                            width: 360
                            height: 380
                            LabelType {
                                x: 10
                                y: 70
                                width: 100
                                height: 20
                                text: qsTr("Password")
                            }
                            LabelType {
                                x: 10
                                y: 10
                                width: 100
                                height: 20
                                text: qsTr("Server:")
                            }
                            LabelType {
                                x: 10
                                y: 50
                                width: 100
                                height: 20
                                text: qsTr("Encryption:")
                            }
                            LabelType {
                                x: 10
                                y: 30
                                width: 100
                                height: 20
                                text: qsTr("Port:")
                            }
                            LabelType {
                                x: 10
                                y: 100
                                width: 191
                                height: 20
                                text: qsTr("Connection string")
                            }
                            LabelType {
                                x: 130
                                y: 70
                                width: 100
                                height: 20
                                text: qsTr("Password")
                            }
                            LabelType {
                                x: 130
                                y: 10
                                width: 100
                                height: 20
                                text: qsTr("Server:")
                            }
                            LabelType {
                                x: 130
                                y: 50
                                width: 100
                                height: 20
                                text: qsTr("Encryption:")
                            }
                            LabelType {
                                x: 130
                                y: 30
                                width: 100
                                height: 20
                                text: qsTr("Port:")
                            }
                            Image {
                                id: label_share_ss_qr_code
                                x: 85
                                y: 235
                                width: 200
                                height: 200

                                //                            source: "file"
                            }
                            ShareConnectionButtonType {
                                x: 10
                                y: 180
                                width: 331
                                height: 40
                                text: qsTr("Copy")
                            }
                            TextFieldType {
                                x: 10
                                y: 130
                                width: 331
                                height: 100
                                horizontalAlignment: Text.AlignHCenter
                            }
                        }
                    }
                    onClicked: {
                        if (active) {
                            active = false
                        } else {
                            ct.clearActive()
                            active = true
                        }
                    }
                }
                ShareConnectionContent {
                    id: share_cloak
                    x: 0
                    text: qsTr("Share for Cloak client")
                    content: Component {
                        Item {
                            width: 360
                            height: 380
                            ShareConnectionButtonType {
                                x: 10
                                y: 290
                                width: 331
                                height: 40
                                text: qsTr("Copy")
                            }
                            TextInput {
                                x: 10
                                y: 30
                                width: 331
                                height: 100
                            }
                        }
                    }
                    onClicked: {
                        if (active) {
                            active = false
                        } else {
                            ct.clearActive()
                            active = true
                        }
                    }
                }
            }
        }
    }
}
