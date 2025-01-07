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

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20
    }

    ListView {
        id: listView
        anchors.top: backButton.bottom
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.left: parent.left

        property bool isFocusable: true

        ScrollBar.vertical: ScrollBarType {}

        header: ColumnLayout {
            width: listView.width

            HeaderType {
                id: header

                Layout.fillWidth: true
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                headerText: "Dev menu"
            }
        }
        
        model: 1
        clip: true
        spacing: 16

        delegate: ColumnLayout {
            width: listView.width

            TextFieldWithHeaderType {
                id: passwordTextField

                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.rightMargin: 16
                Layout.leftMargin: 16

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
        }

        footer: ColumnLayout {
            width: listView.width

            SwitcherType {
                id: switcher

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
