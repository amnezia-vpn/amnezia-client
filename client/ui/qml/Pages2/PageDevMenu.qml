import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20 + PageController.safeAreaTopMargin
    }

    ListViewType {
        id: listView
        anchors.top: backButton.bottom
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.left: parent.left

        header: ColumnLayout {
            width: listView.width

            BaseHeaderType {
                Layout.fillWidth: true
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                headerText: "Dev menu"
            }
        }
        
        model: 1 // fake model to force the ListView to be created without a model

        spacing: 16

        delegate: ColumnLayout {
            width: listView.width

            TextFieldWithHeaderType {
                id: gatewayEndpointField

                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                headerText: qsTr("Gateway endpoint")
                textField.text: SettingsController.gatewayEndpoint

                buttonImageSource: textField.text !== "" ? "qrc:/images/controls/refresh-cw.svg" : ""

                clickedFunc: function() {
                    SettingsController.resetGatewayEndpoint()
                    gatewayEndpointField.textField.text = SettingsController.gatewayEndpoint
                }
            }

            BasicButtonType {
                id: saveButton

                Layout.fillWidth: true
                Layout.margins: 16

                text: qsTr("Save")

                clickedFunc: function() {
                    var trimmed = gatewayEndpointField.textField.text.replace(/^\s+|\s+$/g, '')
                    gatewayEndpointField.textField.text = trimmed
                    if (trimmed !== SettingsController.gatewayEndpoint) {
                        SettingsController.gatewayEndpoint = trimmed
                    }
                    PageController.showNotificationMessage(qsTr("Settings saved"))
                }
            }
        }

        footer: ColumnLayout {
            width: listView.width

            SwitcherType {
                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                text: qsTr("Dev gateway environment")
                checked: SettingsController.isDevGatewayEnv
                onToggled: function() {
                    SettingsController.isDevGatewayEnv = checked
                }
            }
        }
    }
}
