import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.15
import PageEnum 1.0
import "./"
import "../Controls"
import "../Config"

PageBase {
    id: root
    page: PageEnum.AppSettings
    logic: AppSettingsLogic

    BackButton {
        id: back
    }
    Caption {
        id: caption
        text: qsTr("Application Settings")
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

            CheckBoxType {
                Layout.fillWidth: true
                text: qsTr("Auto connect")
                checked: AppSettingsLogic.checkBoxAutoConnectChecked
                onCheckedChanged: {
                    AppSettingsLogic.checkBoxAutoConnectChecked = checked
                    AppSettingsLogic.onCheckBoxAutoconnectToggled(checked)
                }
            }
            CheckBoxType {
                Layout.fillWidth: true
                text: qsTr("Auto start")
                checked: AppSettingsLogic.checkBoxAutostartChecked
                onCheckedChanged: {
                    AppSettingsLogic.checkBoxAutostartChecked = checked
                    AppSettingsLogic.onCheckBoxAutostartToggled(checked)
                }
            }
            CheckBoxType {
                Layout.fillWidth: true
                text: qsTr("Start minimized")
                checked: AppSettingsLogic.checkBoxStartMinimizedChecked
                onCheckedChanged: {
                    AppSettingsLogic.checkBoxStartMinimizedChecked = checked
                    AppSettingsLogic.onCheckBoxStartMinimizedToggled(checked)
                }
            }
            LabelType {
                Layout.fillWidth: true
                Layout.topMargin: 15
                text: AppSettingsLogic.labelVersionText
            }
            BlueButtonType {
                Layout.fillWidth: true
                Layout.preferredHeight: 41
                text: qsTr("Check for updates")
                onClicked: {
                    Qt.openUrlExternally("https://github.com/amnezia-vpn/desktop-client/releases/latest")
                }
            }
            BlueButtonType {
                Layout.fillWidth: true
                Layout.topMargin: 15
                Layout.preferredHeight: 41
                text: qsTr("Open logs folder")
                onClicked: {
                    AppSettingsLogic.onPushButtonOpenLogsClicked()
                }
            }

            BlueButtonType {
                Layout.fillWidth: true
                Layout.topMargin: 15
                Layout.preferredHeight: 41
                text: qsTr("Export logs")
                onClicked: {
                    AppSettingsLogic.onPushButtonOpenLogsClicked()
                }
            }

        }
    }

    Logo {
        id: logo
        anchors.bottom: parent.bottom
    }
}
