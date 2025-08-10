import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "../Controls2"
import "../Controls2/TextTypes"

import "../Config"

DrawerType2 {
    property string serverNameText

    id: root
    objectName: "serverNameEditDrawer"

    expandedStateContent: ColumnLayout {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 32
        anchors.leftMargin: 16
        anchors.rightMargin: 16

        TextFieldWithHeaderType {
            id: serverName

            Layout.fillWidth: true
            headerText: qsTr("Server name")
            textField.text: root.serverNameText
            textField.maximumLength: 30
            checkEmptyText: true
        }

        BasicButtonType {
            id: saveButton

            Layout.fillWidth: true

            text: qsTr("Save")

            clickedFunc: function() {
                if (serverName.textField.text === "") {
                    return
                }

                if (serverName.textField.text !== root.serverNameText) {
                    ServersModel.setProcessedServerData("name", serverName.textField.text);
                }
                root.closeTriggered()
            }
        }
    }
}
