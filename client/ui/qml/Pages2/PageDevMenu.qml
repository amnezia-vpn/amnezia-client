import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    ColumnLayout {
        id: backButtonLayout

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        anchors.topMargin: 20

        BackButtonType {
            id: backButton
        }
    }

    FlickableType {
        id: fl
        anchors.top: backButtonLayout.bottom
        anchors.bottom: parent.bottom
        contentHeight: content.implicitHeight

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            HeaderType {
                id: header

                Layout.fillWidth: true
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                headerText: "Dev menu"
            }

            TextFieldWithHeaderType {
                id: passwordTextField

                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.rightMargin: 16
                Layout.leftMargin: 16
                parentFlickable: fl

                headerText: qsTr("Gateway endpoint")
                textFieldText: SettingsController.gatewayEndpoint

                buttonImageSource: textFieldText !== "" ? "qrc:/images/controls/refresh-cw.svg" : ""

                clickedFunc: function() {
                    SettingsController.resetGatewayEndpoint()
                }

                textField.onEditingFinished: {
                    textFieldText = textField.text.replace(/^\s+|\s+$/g, '')
                    if (textFieldText !== SettingsController.gatewayEndpoint) {
                        SettingsController.gatewayEndpoint = textFieldText
                    }
                }
            }

            SwitcherType {
                id: switcher

                Layout.fillWidth: true
                Layout.rightMargin: 16
                Layout.leftMargin: 16
                Layout.topMargin: 16

                text: qsTr("Dev gateway environment")
                checked: SettingsController.isDevGatewayEnv
                onToggled: function() {
                    SettingsController.isDevGatewayEnv = checked
                }
            }
        }
    }
}
