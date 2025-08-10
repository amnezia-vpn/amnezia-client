pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "../Controls2"
import "../Controls2/TextTypes"

import "../Config"

DrawerType2 {
    id: root

    Connections {
        target: ApiPremV1MigrationController

        function onOtpSuccessfullySent() {
            root.openTriggered()
        }
    }

    expandedHeight: parent.height * 0.6

    expandedStateContent: Item {
        implicitHeight: root.expandedHeight

        ColumnLayout {

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            spacing: 0

            Header2Type {
                id: header
                Layout.fillWidth: true
                Layout.topMargin: 20

                headerText: qsTr("OTP code was sent to your email")
            }

            TextFieldWithHeaderType {
                id: otpFiled

                borderColor: AmneziaStyle.color.mutedGray
                headerTextColor: AmneziaStyle.color.paleGray

                Layout.fillWidth: true
                Layout.topMargin: 16
                headerText: qsTr("OTP Code")
                textField.maximumLength: 30
                checkEmptyText: true
            }

            BasicButtonType {
                id: saveButton

                Layout.fillWidth: true
                Layout.topMargin: 16

                text: qsTr("Continue")

                clickedFunc: function() {
                    PageController.showBusyIndicator(true)
                    ApiPremV1MigrationController.migrate(otpFiled.textField.text)
                    PageController.showBusyIndicator(false)
                    root.closeTriggered()
                }
            }
        }
    }
}
