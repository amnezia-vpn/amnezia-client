import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.15
import PageEnum 1.0
import "./"
import "../Controls"
import "../Config"

PageBase {
    id: root
    page: PageEnum.ServerSettings
    logic: ServerSettingsLogic

    enabled: ServerSettingsLogic.pageEnabled

    BackButton {
        id: back
    }
    Caption {
        id: caption
        text: qsTr("Server settings")
        anchors.horizontalCenter: parent.horizontalCenter
    }

    FlickableType {
        id: fl
        anchors.top: caption.bottom
        anchors.bottom: logo.top
        contentHeight: content.height

        ColumnLayout {
            id: content
            enabled: logic.pageEnabled
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.rightMargin: 15

            LabelType {
                Layout.fillWidth: true
                font.pixelSize: 20
                horizontalAlignment: Text.AlignHCenter
                text: ServerSettingsLogic.labelCurrentVpnProtocolText
            }

            TextFieldType {
                Layout.fillWidth: true
                font.pixelSize: 20
                horizontalAlignment: Text.AlignHCenter
                text: ServerSettingsLogic.labelServerText
                readOnly: true
                background: Item {}
            }

            LabelType {
                Layout.fillWidth: true
                text: ServerSettingsLogic.labelWaitInfoText
                visible: ServerSettingsLogic.labelWaitInfoVisible
            }
            TextFieldType {
                Layout.fillWidth: true
                text: ServerSettingsLogic.lineEditDescriptionText
                onEditingFinished: {
                    ServerSettingsLogic.lineEditDescriptionText = text
                    ServerSettingsLogic.onLineEditDescriptionEditingFinished()
                }
            }

            BlueButtonType {
                text: qsTr("Protocols and Services")
                Layout.topMargin: 20
                Layout.fillWidth: true
                onClicked: {
                    UiLogic.goToPage(PageEnum.ServerContainers)
                }
            }
            BlueButtonType {
                Layout.fillWidth: true
                text: qsTr("Share Server (FULL ACCESS)")
                visible: ServerSettingsLogic.pushButtonShareFullVisible
                onClicked: {
                    ServerSettingsLogic.onPushButtonShareFullClicked()
                }
            }

            BlueButtonType {
                Layout.fillWidth: true
                Layout.topMargin: 60
                text: ServerSettingsLogic.pushButtonClearText
                visible: ServerSettingsLogic.pushButtonClearVisible
                onClicked: {
                    ServerSettingsLogic.onPushButtonClearServer()
                }
            }
            BlueButtonType {
                Layout.fillWidth: true
                text: ServerSettingsLogic.pushButtonClearClientCacheText
                visible: ServerSettingsLogic.pushButtonClearClientCacheVisible
                onClicked: {
                    ServerSettingsLogic.onPushButtonClearClientCacheClicked()
                }
            }
            BlueButtonType {
                Layout.fillWidth: true
                text: qsTr("Forget this server")
                onClicked: {
                    ServerSettingsLogic.onPushButtonForgetServer()
                }
            }

        }
    }


    Logo {
        id : logo
        anchors.bottom: parent.bottom
    }
}
