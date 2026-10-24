import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "../Controls2"
import "../Controls2/TextTypes"

import "../Config"

SheetType {
    property string serverNameText

    id: root

    expandedHeight: content.implicitHeight + 16 + 8 + PageController.safeAreaBottomMargin + PageController.imeHeight

    ColumnLayout {
        id: content

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 16
        anchors.leftMargin: 16
        anchors.rightMargin: 16

        spacing: 24

        TextFieldType {
            id: serverName

            Layout.fillWidth: true
            headerText: qsTr("Server name")
            textField.text: root.serverNameText
            textField.maximumLength: 30
            checkEmptyText: true
        }

        ButtonType {
            id: saveButton

            Layout.fillWidth: true

            text: qsTr("Save")

            onClicked: {
                if (serverName.textField.text === "") {
                    return
                }

                if (serverName.textField.text !== root.serverNameText) {
                    ServersUiController.editServerName(ServersUiController.processedServerId, serverName.textField.text);
                }
                root.close()
            }
        }
    }
}
