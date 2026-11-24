import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    property var validation: ({ valid: false, line: 0, message: "" })

    readonly property bool hasJson: jsonArea.textAreaText.trim() !== ""
    readonly property bool canAdd: root.hasJson && root.validation.valid && nameField.textField.text.trim() !== ""

    function revalidate() {
        root.validation = root.hasJson ? XraySettingsModel.validate(jsonArea.textAreaText)
                                       : { valid: false, line: 0, message: "" }
        console.log("[XrayUI][ImportPreset] validated, length", jsonArea.textAreaText.length, "valid", root.validation.valid, "line", root.validation.line, root.validation.message)
    }

    Component.onCompleted: console.log("[XrayUI][ImportPreset] opened")
    Component.onDestruction: console.log("[XrayUI][ImportPreset] closed")

    FlickableType {
        id: flickable

        anchors.fill: parent

        contentHeight: content.implicitHeight + 16 + PageController.safeAreaBottomMargin + PageController.imeHeight

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 0

            NavigationBarType {
                Layout.fillWidth: true

                title: qsTr("Import preset")
            }

            TextFieldType {
                id: nameField

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("Name")
                textField.maximumLength: 60

                textField.onEditingFinished: console.log("[XrayUI][ImportPreset] name edited", nameField.textField.text)
            }

            SmallTextType {
                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 12

                text: qsTr("Preset JSON")
            }

            TextAreaType {
                id: jsonArea

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                placeholderText: "{ \"port\": 443, \"protocol\": \"vless\", … }"
                errorText: root.hasJson && !root.validation.valid ? root.validation.message : ""

                textArea.font.family: GC.monospaceFontFamily
                textArea.font.pixelSize: 13
                textArea.font.letterSpacing: 0
                textArea.inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText | Qt.ImhPreferLatin
                textArea.onTextChanged: root.revalidate()
            }

            ListItemType {
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Choose .json file")
                trailingIconSource: "qrc:/images/controls/chevron-right.svg"

                onClicked: {
                    const filePath = SystemController.getFileName(qsTr("Open XRay preset"), qsTr("JSON files (*.json)"))
                    console.log("[XrayUI][ImportPreset] file chosen", filePath)
                    if (filePath === "") {
                        return
                    }
                    const content = ImportController.readTextFile(filePath)
                    console.log("[XrayUI][ImportPreset] file read, length", content.length)
                    if (content === "") {
                        PageController.showNotificationMessage(qsTr("Could not read the file"))
                        return
                    }
                    jsonArea.textAreaText = content
                    if (nameField.textField.text.trim() === "") {
                        const parts = filePath.split(/[\\/]/)
                        nameField.textField.text = parts[parts.length - 1].replace(/\.json$/i, "")
                    }
                }
            }

            ButtonType {
                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                enabled: root.canAdd
                text: qsTr("Add preset")

                onClicked: {
                    const name = nameField.textField.text.trim()
                    const id = XrayPresetsModel.addPreset(name, jsonArea.textAreaText)
                    console.log("[XrayUI][ImportPreset] preset added", id, name)
                    PageController.showNotificationMessage(qsTr("Preset added"))
                    PageController.closePage()
                }
            }
        }
    }
}
