import QtQuick 2.12
import QtQuick.Controls 2.12
import PageEnum 1.0
import "./"
import "../Controls"
import "../Config"

PageBase {
    id: root
    page: PageEnum.Start
    logic: StartPageLogic

    BackButton {
        id: back_from_start
    }

    Caption {
        id: caption
        text: start_switch_page.checked ?
                  qsTr("Setup your server to use VPN") :
                  qsTr("Connect to the already created VPN server")
    }

    Logo {
        id: logo
        anchors.bottom: parent.bottom
    }

    BasicButtonType {
        id: start_switch_page
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: logo.top
        anchors.bottomMargin: 10

        width: parent.width - 80
        height: 40
        anchors.topMargin: 20

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
        width: parent.width
        anchors.top: caption.bottom
        anchors.bottom: start_switch_page.top
        anchors.bottomMargin: 10

        visible: true

        LabelType {
            id: label_connection_code
            anchors.top: parent.top
            anchors.topMargin: 20
            text: qsTr("Connection code")
        }
        TextFieldType {
            id: lineEdit_start_existing_code
            anchors.top: label_connection_code.bottom
            anchors.horizontalCenter: parent.horizontalCenter
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
            anchors.top: lineEdit_start_existing_code.bottom
            anchors.topMargin: 40
            text: qsTr("Connect")
            onClicked: {
                StartPageLogic.onPushButtonImport()
            }
        }
    }


    Item {
        id: page_start_new_server
        width: parent.width
        anchors.top: caption.bottom
        anchors.bottom: start_switch_page.top
        anchors.bottomMargin: 10

        visible: false

        BasicButtonType {
            id: new_sever_get_info
            width: parent.width - 80
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 5

            text: qsTr("How to get own server? →")
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
                Qt.openUrlExternally("https://amnezia.org/instruction.html")
            }
        }
        LabelType {
            id: label_server_ip
            x: 40
            anchors.top: new_sever_get_info.bottom
            text: qsTr("Server IP address")
        }
        TextFieldType {
            id: new_server_ip
            anchors.top: label_server_ip.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            text: StartPageLogic.lineEditIpText
            onEditingFinished: {
                StartPageLogic.lineEditIpText = text
            }
        }

        LabelType {
            id:label_login
            x: 40
            anchors.top: new_server_ip.bottom
            text: qsTr("Login to connect via SSH")
        }
        TextFieldType {
            id: new_server_login
            anchors.top: label_login.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            text: StartPageLogic.lineEditLoginText
            onEditingFinished: {
                StartPageLogic.lineEditLoginText = text
            }
        }

        LabelType {
            id: label_new_server_password
            x: 40
            anchors.top: new_server_login.bottom
            text: qsTr("Password")
        }
        TextFieldType {
            id: new_server_password
            anchors.top: label_new_server_password.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            echoMode: TextInput.Password
            text: StartPageLogic.lineEditPasswordText
            onEditingFinished: {
                StartPageLogic.lineEditPasswordText = text
            }
        }
        TextFieldType {
            id: new_server_ssh_key
            anchors.top: label_new_server_password.bottom
            anchors.horizontalCenter: parent.horizontalCenter

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

        LabelType {
            x: 40
            y: 390
            width: 301
            height: 41
            text: StartPageLogic.labelWaitInfoText
            visible: StartPageLogic.labelWaitInfoVisible
            wrapMode: Text.Wrap
        }



        BlueButtonType {
            id: new_sever_connect
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: new_server_ssh_key.bottom

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
            anchors.top: new_sever_connect.bottom

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
    }
}
