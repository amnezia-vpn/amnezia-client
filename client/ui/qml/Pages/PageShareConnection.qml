import QtQuick 2.12
import QtQuick.Controls 2.12
import "./"
import "../Controls"
import "../Config"

Item {
    id: root
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
            property int currentIndex: UiLogic.toolBoxShareConnectionCurrentIndex
            onCurrentIndexChanged: {
                UiLogic.toolBoxShareConnectionCurrentIndex = currentIndex
                for (let i = 0; i < contentList.length; ++i) {
                    if (i == currentIndex) {
                        contentList[i].active = true
                    } else {
                        contentList[i].active = false
                    }
                }
            }

            function clearActive() {
                for (let i = 0; i < contentList.length; ++i) {
                    contentList[i].active = false
                }
                currentIndex = -1;
            }
            Column {
                spacing: 5
                ShareConnectionContent {
                    id: full_access
                    x: 0
                    text: qsTr("Full access")
                    visible: UiLogic.pageShareFullAccessVisible
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
                                text: UiLogic.pushButtonShareFullCopyText
                                onClicked: {
                                    UiLogic.onPushButtonShareFullCopyClicked()
                                }
                            }
                            ShareConnectionButtonType {
                                x: 10
                                y: 180
                                width: 341
                                height: 40
                                text: qsTr("Save file")
                                onClicked: {
                                    UiLogic.onPushButtonShareFullSaveClicked()
                                }
                            }
                            TextFieldType {
                                x: 10
                                y: 10
                                width: 341
                                height: 100
                                verticalAlignment: Text.AlignTop
                                text: UiLogic.textEditShareFullCodeText
                                onEditingFinished: {
                                    UiLogic.textEditShareFullCodeText = text
                                }
                            }
                        }
                    }
                    onClicked: {
                        if (active) {
                            ct.currentIndex = -1
                        } else {
                            ct.clearActive()
                            ct.currentIndex = 0
                        }
                    }
                }
                ShareConnectionContent {
                    id: share_amezia
                    x: 0
                    text: qsTr("Share for Amnezia client")
                    visible: UiLogic.pageShareAmneziaVisible
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
                                text: UiLogic.pushButtonShareAmneziaCopyText
                                onClicked: {
                                    UiLogic.onPushButtonShareAmneziaCopyClicked()
                                }
                                enabled: UiLogic.pushButtonShareAmneziaCopyEnabled
                            }
                            ShareConnectionButtonType {
                                x: 10
                                y: 130
                                width: 341
                                height: 40
                                text: UiLogic.pushButtonShareAmneziaGenerateText
                                enabled: UiLogic.pushButtonShareAmneziaGenerateEnabled
                                onClicked: {
                                    UiLogic.onPushButtonShareAmneziaGenerateClicked()
                                }
                            }
                            ShareConnectionButtonType {
                                x: 10
                                y: 230
                                width: 341
                                height: 40
                                text: qsTr("Save file")
                                onClicked: {
                                    UiLogic.onPushButtonShareAmneziaSaveClicked()
                                }
                            }
                            TextFieldType {
                                x: 10
                                y: 10
                                width: 341
                                height: 100
                                verticalAlignment: Text.AlignTop
                                text: UiLogic.textEditShareAmneziaCodeText
                                onEditingFinished: {
                                    UiLogic.textEditShareAmneziaCodeText = text
                                }
                            }
                        }
                    }
                    onClicked: {
                        if (active) {
                            ct.currentIndex = -1
                        } else {
                            ct.clearActive()
                            ct.currentIndex = 1
                        }
                    }
                }
                ShareConnectionContent {
                    id: share_openvpn
                    x: 0
                    text: qsTr("Share for OpenVPN client")
                    visible: UiLogic.pageShareOpenvpnVisible
                    content: Component {
                        Item {
                            width: 360
                            height: 380
                            ShareConnectionButtonType {
                                x: 10
                                y: 180
                                width: 341
                                height: 40
                                text: UiLogic.pushButtonShareOpenvpnCopyText
                                enabled: UiLogic.pushButtonShareOpenvpnCopyEnabled
                                onClicked: {
                                    UiLogic.onPushButtonShareOpenvpnCopyClicked()
                                }
                            }
                            ShareConnectionButtonType {
                                x: 10
                                y: 130
                                width: 341
                                height: 40
                                text: UiLogic.pushButtonShareOpenvpnGenerateText
                                onClicked: {
                                    UiLogic.onPushButtonShareOpenvpnGenerateClicked()
                                }
                                enabled: UiLogic.pushButtonShareOpenvpnGenerateEnabled
                            }
                            ShareConnectionButtonType {
                                x: 10
                                y: 230
                                width: 341
                                height: 40
                                text: qsTr("Save file")
                                enabled: UiLogic.pushButtonShareOpenvpnSaveEnabled
                                onClicked: {
                                    UiLogic.onPushButtonShareOpenvpnSaveClicked()
                                }
                            }
                            TextFieldType {
                                x: 10
                                y: 10
                                width: 341
                                height: 100
                                verticalAlignment: Text.AlignTop
                                text: UiLogic.textEditShareOpenvpnCodeText
                                onEditingFinished: {
                                    UiLogic.textEditShareOpenvpnCodeText = text
                                }
                            }
                        }
                    }
                    onClicked: {
                        if (active) {
                            ct.currentIndex = -1
                        } else {
                            ct.clearActive()
                            ct.currentIndex = 2
                        }
                    }
                }
                ShareConnectionContent {
                    id: share_shadowshock
                    x: 0
                    text: qsTr("Share for ShadowSocks client")
                    visible: UiLogic.pageShareShadowsocksVisible
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
                                text: UiLogic.labelShareSsPasswordText
                            }
                            LabelType {
                                x: 130
                                y: 10
                                width: 100
                                height: 20
                                text: UiLogic.labelShareSsServerText
                            }
                            LabelType {
                                x: 130
                                y: 50
                                width: 100
                                height: 20
                                text: UiLogic.labelShareSsMethodText
                            }
                            LabelType {
                                x: 130
                                y: 30
                                width: 100
                                height: 20
                                text: UiLogic.labelShareSsPortText
                            }
                            Image {
                                id: label_share_ss_qr_code
                                x: 85
                                y: 235
                                width: 200
                                height: 200
                                source: UiLogic.labelShareSsQrCodeText == "" ? "" : "data:image/png;base64," + UiLogic.labelShareSsQrCodeText
                            }
                            ShareConnectionButtonType {
                                x: 10
                                y: 180
                                width: 331
                                height: 40
                                text: UiLogic.pushButtonShareSsCopyText
                                enabled: UiLogic.pushButtonShareSsCopyEnabled
                                onClicked: {
                                    UiLogic.onPushButtonShareSsCopyClicked()
                                }
                            }
                            TextFieldType {
                                x: 10
                                y: 130
                                width: 331
                                height: 100
                                horizontalAlignment: Text.AlignHCenter
                                text: UiLogic.lineEditShareSsStringText
                                onEditingFinished: {
                                    UiLogic.lineEditShareSsStringText = text
                                }
                            }
                        }
                    }
                    onClicked: {
                        if (active) {
                            ct.currentIndex = -1
                        } else {
                            ct.clearActive()
                            ct.currentIndex = 3
                        }
                    }
                }
                ShareConnectionContent {
                    id: share_cloak
                    x: 0
                    text: qsTr("Share for Cloak client")
                    visible: UiLogic.pageShareCloakVisible
                    content: Component {
                        Item {
                            width: 360
                            height: 380
                            ShareConnectionButtonType {
                                x: 10
                                y: 290
                                width: 331
                                height: 40
                                text: UiLogic.pushButtonShareCloakCopyText
                                enabled: UiLogic.pushButtonShareCloakCopyEnabled
                                onClicked: {
                                    UiLogic.onPushButtonShareCloakCopyClicked()
                                }
                            }
                            TextInput {
                                x: 10
                                y: 30
                                width: 331
                                height: 100
                                text: UiLogic.plainTextEditShareCloakText
                                onEditingFinished: {
                                    UiLogic.plainTextEditShareCloakText = text
                                }
                            }
                        }
                    }
                    onClicked: {
                        if (active) {
                            ct.currentIndex = -1
                        } else {
                            ct.clearActive()
                            ct.currentIndex = 4
                        }
                    }
                }
            }
        }
    }
}
