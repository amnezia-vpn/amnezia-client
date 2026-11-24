import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "../Controls2"
import "../Config"

SheetType {
    id: root

    property string defaultName

    signal saveRequested(string name)

    expandedHeight: content.implicitHeight + 16 + 8 + PageController.safeAreaBottomMargin + PageController.imeHeight

    onOpened: {
        console.log("[XrayUI][SavePresetSheet] opened, default name", root.defaultName)
        nameField.textField.text = root.defaultName
    }
    onClosed: console.log("[XrayUI][SavePresetSheet] closed")

    ColumnLayout {
        id: content

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 16
        anchors.leftMargin: 16
        anchors.rightMargin: 16

        spacing: 16

        TextFieldType {
            id: nameField

            Layout.fillWidth: true

            headerText: qsTr("Name")
            textField.maximumLength: 60
            checkEmptyText: true
        }

        ButtonType {
            Layout.fillWidth: true

            enabled: nameField.textField.text.trim() !== ""
            text: qsTr("Save")

            onClicked: {
                const name = nameField.textField.text.trim()
                console.log("[XrayUI][SavePresetSheet] save clicked, name", name)
                root.saveRequested(name)
                root.close()
            }
        }
    }
}
