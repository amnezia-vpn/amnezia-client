import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PageEnum 1.0
import "./"
import "../Controls"
import "../Config"

PageBase {
    id: root
    page: PageEnum.AdvancedServerSettings
    logic: AdvancedServerSettingsLogic

    enabled: AdvancedServerSettingsLogic.pageEnabled

    BackButton {
        id: back
    }

    Caption {
        id: caption
        text: qsTr("Advanced server settings")
        anchors.horizontalCenter: parent.horizontalCenter
    }

    BusyIndicator {
        z: 99
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        visible: !AdvancedServerSettingsLogic.pageEnabled
        running: !AdvancedServerSettingsLogic.pageEnabled
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
                text: AdvancedServerSettingsLogic.labelCurrentVpnProtocolText
            }

            TextFieldType {
                Layout.fillWidth: true
                font.pixelSize: 20
                horizontalAlignment: Text.AlignHCenter
                text: AdvancedServerSettingsLogic.labelServerText
                readOnly: true
                background: Item {}
            }

            LabelType {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                text: AdvancedServerSettingsLogic.labelWaitInfoText
                visible: AdvancedServerSettingsLogic.labelWaitInfoVisible
            }

            BlueButtonType {
                Layout.fillWidth: true
                Layout.topMargin: 10
                text: "Scan the server for installed containers"
                visible: AdvancedServerSettingsLogic.pushButtonClearVisible
                onClicked: {
                    AdvancedServerSettingsLogic.onPushButtonScanServerClicked()
                }
            }

            BlueButtonType {
                Layout.fillWidth: true
                Layout.topMargin: 10
                text: AdvancedServerSettingsLogic.pushButtonClearText
                visible: AdvancedServerSettingsLogic.pushButtonClearVisible
                onClicked: {
                    popupClearServer.open()
                }
            }

            BlueButtonType {
                Layout.topMargin: 10
                Layout.fillWidth: true
                text: qsTr("Clients Management")
                onClicked: {
                    UiLogic.goToPage(PageEnum.ClientManagement)
                }
            }

            PopupWithQuestion {
                id: popupClearServer
                questionText: "Attention! All containers will be deleted on the server. This means that configuration files, keys and certificates will be deleted. Continue?"
                yesFunc: function() {
                    close()
                    AdvancedServerSettingsLogic.onPushButtonClearServerClicked()
                }
                noFunc: function() {
                    close()
                }
            }
        }
    }

    Logo {
        id : logo
        anchors.bottom: parent.bottom
    }
}
