import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Config"
import "../Components"

PageType {
    id: root

    FlickableType {
        id: flickable

        anchors.top: parent.top

        contentHeight: content.implicitHeight

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 0

            NavigationBarType {
                Layout.fillWidth: true

                title: "Dev menu"
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 16 + PageController.safeAreaBottomMargin

                spacing: 24

                ColumnLayout {
                    Layout.fillWidth: true

                    spacing: 12

                    TextFieldType {
                        id: gatewayEndpointField

                        Layout.fillWidth: true

                        headerText: qsTr("Gateway endpoint")
                        textField.text: SettingsController.devGatewayEndpoint
                    }

                    TextFieldType {
                        id: gatewayProxyStoragesField

                        Layout.fillWidth: true

                        headerText: qsTr("Gateway proxy storages")
                        textField.text: SettingsController.devGatewayProxyStorages
                    }

                    TextFieldType {
                        id: gatewayPublicKeyField

                        Layout.fillWidth: true

                        headerText: qsTr("Gateway public key")
                        textField.text: SettingsController.devGatewayPublicKey
                    }

                    ButtonType {
                        id: saveButton

                        Layout.fillWidth: true

                        text: qsTr("Save")

                        onClicked: {
                            SettingsController.saveDevGatewayConfig(gatewayEndpointField.textField.text.trim(),
                                                                    gatewayProxyStoragesField.textField.text.trim(),
                                                                    gatewayPublicKeyField.textField.text.trim())
                            gatewayEndpointField.textField.text = SettingsController.devGatewayEndpoint
                            gatewayProxyStoragesField.textField.text = SettingsController.devGatewayProxyStorages
                            gatewayPublicKeyField.textField.text = SettingsController.devGatewayPublicKey
                            PageController.showNotificationMessage(qsTr("Settings saved"))
                        }
                    }
                }

                ListItemType {
                    id: devGatewayRow

                    Layout.fillWidth: true

                    text: qsTr("Dev gateway environment")

                    onClicked: {
                        devGatewaySwitch.toggle()
                        devGatewaySwitch.toggled()
                    }

                    SwitchType {
                        id: devGatewaySwitch

                        focusPolicy: Qt.NoFocus

                        checked: SettingsController.isDevGatewayEnv
                        onToggled: function() {
                            SettingsController.isDevGatewayEnv = checked
                        }
                    }
                }
            }
        }
    }
}
