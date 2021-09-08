import QtQuick 2.12
import QtQuick.Controls 2.12
import "./"
import "../Controls"
import "../Config"

Item {
    id: root
    BackButton {
        id: back_from_start
    }
    Image {
        anchors.horizontalCenter: root.horizontalCenter
        width: GC.trW(150)
        height: GC.trH(22)
        y: GC.trY(590)
        source: "qrc:/images/AmneziaVPN.png"
    }

    BasicButtonType {
        id: start_switch_page
        anchors.horizontalCenter: parent.horizontalCenter
        y: 530
        width: 301
        height: 40
        text: qsTr("Set up your own server")
        checked: false
        checkable: true
        onCheckedChanged: {
            if (checked) {
                page_start_new_server.visible = true
                page_start_import.visible = false
                text = qsTr("Import connection");
            }
            else {
                page_start_new_server.visible = false
                page_start_import.visible = true
                text = qsTr("Set up your own server");
            }
        }

        background: Rectangle {
            anchors.fill: parent
            border.width: 1
            border.color: "#211C4A"
            radius: 4
        }

        contentItem: Text {
            anchors.fill: parent
            font.family: "Lato"
            font.styleName: "normal"
            font.pixelSize: 16
            color: "#100A44"
            text: start_switch_page.text
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        antialiasing: true

    }


        Item {
            id: page_start_import
            width: 380
            height: 481
            x: 0
            y: 35
            visible: true
            Text {
                x: 0
                y: 20
                width: 381
                height: 71
                font.family: "Lato"
                font.styleName: "normal"
                font.pixelSize: 24
                font.bold: true
                color: "#100A44"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                text: qsTr("Connect to the already created VPN server")
                wrapMode: Text.Wrap
            }
            LabelType {
                x: 40
                y: 110
                width: 301
                height: 21
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
                text: qsTr("Connection code")
                wrapMode: Text.Wrap
            }
            TextFieldType {
                id: lineEdit_start_existing_code
                x: 40
                y: 140
                width: 300
                height: 40
                placeholderText: "vpn://..."
                text: StartPageLogic.lineEditStartExistingCodeText
                onEditingFinished: {
                    StartPageLogic.lineEditStartExistingCodeText = text
                }
            }
            BlueButtonType {
                id: new_sever_import
                anchors.horizontalCenter: parent.horizontalCenter
                y: 210
                width: 301
                height: 40
                text: qsTr("Connect")
                onClicked: {
                    StartPageLogic.onPushButtonImport()
                }
            }
        }


        Item {
            id: page_start_new_server
            width: 380
            height: 481
            x: 0
            y: 35
            visible: false
            Label {
                x:10
                y:0
                width: 361
                height: 31
                font.family: "Lato"
                font.styleName: "normal"
                font.pixelSize: 24
                font.bold: true
                color: "#100A44"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                text: qsTr("Setup your server to use VPN")
                wrapMode: Text.Wrap
            }
            LabelType {
                x: 40
                y: 70
                width: 171
                height: 21
                text: qsTr("Server IP address")
            }
            LabelType {
                x: 40
                y: 150
                width: 261
                height: 21
                text: qsTr("Login to connect via SSH")
            }
            LabelType {
                id: label_new_server_password
                x: 40
                y: 230
                width: 171
                height: 21
                text: qsTr("Password")
            }
            LabelType {
                x: 40
                y: 390
                width: 301
                height: 41
                text: StartPageLogic.labelWaitInfoText
                visible: StartPageLogic.labelWaitInfoVisible
                wrapMode: Text.Wrap
            }
            TextFieldType {
                id: new_server_ip
                x: 40
                y: 100
                width: 300
                height: 40
                text: StartPageLogic.lineEditIpText
                onEditingFinished: {
                    StartPageLogic.lineEditIpText = text
                }
            }
            TextFieldType {
                id: new_server_login
                x: 40
                y: 180
                width: 300
                height: 40
                text: StartPageLogic.lineEditLoginText
                onEditingFinished: {
                    StartPageLogic.lineEditLoginText = text
                }
            }
            TextFieldType {
                id: new_server_password
                x: 40
                y: 260
                width: 300
                height: 40
                echoMode: TextInput.Password
                text: StartPageLogic.lineEditPasswordText
                onEditingFinished: {
                    StartPageLogic.lineEditPasswordText = text
                }
            }
            BlueButtonType {
                id: new_sever_connect
                anchors.horizontalCenter: parent.horizontalCenter
                y: 350
                width: 301
                height: 40
                text: StartPageLogic.pushButtonConnectText
                visible: StartPageLogic.pushButtonConnectVisible
                onClicked: {
                    StartPageLogic.onPushButtonConnect()
                }
                enabled: StartPageLogic.pushButtonConnectEnabled
            }
            BasicButtonType {
                id: new_sever_connect_key
                anchors.horizontalCenter: parent.horizontalCenter
                y: 450
                width: 281
                height: 21
                text: qsTr("Connect using SSH key")
                background: Item {
                    anchors.fill: parent
                }

                contentItem: Text {
                    anchors.fill: parent
                    font.family: "Lato"
                    font.styleName: "normal"
                    font.pixelSize: 16
                    color: "#15CDCB";
                    text: new_sever_connect_key.text
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                antialiasing: true
                checkable: true
                checked: StartPageLogic.pushButtonConnectKeyChecked
                onCheckedChanged: {
                    StartPageLogic.pushButtonConnectKeyChecked = checked
                    label_new_server_password.text = checked ? qsTr("Private key") : qsTr("Password")
                    new_sever_connect_key.text = checked ? qsTr("Connect using SSH password") : qsTr("Connect using SSH key")
                    new_server_password.visible = !checked
                    new_server_ssh_key.visible = checked
                }
            }
            BasicButtonType {
                id: new_sever_get_info
                anchors.horizontalCenter: parent.horizontalCenter
                y: 40
                width: 281
                height: 21
                text: qsTr("Where to get connection data →")
                background: Item {
                    anchors.fill: parent
                }

                contentItem: Text {
                    anchors.fill: parent
                    font.family: "Lato"
                    font.styleName: "normal"
                    font.pixelSize: 16
                    color: "#15CDCB";
                    text: new_sever_get_info.text
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                antialiasing: true
                checkable: true
                checked: true
                onClicked: {
                    Qt.openUrlExternally("https://amnezia.org")
                }
            }
            TextFieldType {
                id: new_server_ssh_key
                x: 40
                y: 260
                width: 300
                height: 71
                echoMode: TextInput.Password
                font.pixelSize: 9
                verticalAlignment: Text.AlignTop
                text: StartPageLogic.textEditSshKeyText
                onEditingFinished: {
                    StartPageLogic.textEditSshKeyText = text
                }
                visible: false
            }
        }
}
