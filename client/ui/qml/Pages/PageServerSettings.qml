import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
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

    Flickable {
        id: fl
        width: root.width
        anchors.top: caption.bottom
        anchors.topMargin: 20
        anchors.bottom: logo.top
        anchors.bottomMargin: 20
        anchors.left: root.left
        anchors.leftMargin: 30
        anchors.right: root.right
        anchors.rightMargin: 30

        contentHeight: content.height
        clip: true

        ColumnLayout {
            id: content
            enabled: logic.pageEnabled
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            TextFieldType {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 251
                Layout.preferredHeight: 31
                text: ServerSettingsLogic.lineEditDescriptionText
                onEditingFinished: {
                    ServerSettingsLogic.lineEditDescriptionText = text
                    ServerSettingsLogic.onLineEditDescriptionEditingFinished()
                }
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
                font.pixelSize: 20
                horizontalAlignment: Text.AlignHCenter
                text: ServerSettingsLogic.labelCurrentVpnProtocolText
            }

            BlueButtonType {
                Layout.topMargin: 15
                Layout.fillWidth: true
                Layout.preferredHeight: 41
                text: qsTr("Protocols and Services")
                onClicked: {
                    UiLogic.goToPage(PageEnum.ServerContainers)
                }
            }

            BlueButtonType {
                Layout.topMargin: 10
                Layout.fillWidth: true
                Layout.preferredHeight: 41
                text: qsTr("Share Server (FULL ACCESS)")
                visible: ServerSettingsLogic.pushButtonShareFullVisible
                onClicked: {
                    ServerSettingsLogic.onPushButtonShareFullClicked()
                }
            }

            BlueButtonType {
                Layout.topMargin: 10
                Layout.fillWidth: true
                Layout.preferredHeight: 41
                text: qsTr("Clients Management")
                onClicked: {
                    UiLogic.goToPage(PageEnum.ClientManagement)
                }
            }

            BlueButtonType {
                Layout.topMargin: 30
                Layout.fillWidth: true
                Layout.preferredHeight: 41
                text: ServerSettingsLogic.pushButtonClearClientCacheText
                visible: ServerSettingsLogic.pushButtonClearClientCacheVisible
                onClicked: {
                    ServerSettingsLogic.onPushButtonClearClientCacheClicked()
                }
            }

            BlueButtonType {
                Layout.topMargin: 10
                Layout.fillWidth: true
                Layout.preferredHeight: 41
                text: ServerSettingsLogic.pushButtonClearText
                visible: ServerSettingsLogic.pushButtonClearVisible
                onClicked: {
                    ServerSettingsLogic.onPushButtonClearServer()
                }
            }

            BlueButtonType {
                Layout.topMargin: 10
                Layout.fillWidth: true
                Layout.preferredHeight: 41
                text: qsTr("Forget this server")
                onClicked: {
                    ServerSettingsLogic.onPushButtonForgetServer()
                }
            }

            LabelType {
                Layout.fillWidth: true
                text: ServerSettingsLogic.labelWaitInfoText
                visible: ServerSettingsLogic.labelWaitInfoVisible
            }
        }
    }

    Logo {
        id: logo
        anchors.bottom: parent.bottom
    }
}
