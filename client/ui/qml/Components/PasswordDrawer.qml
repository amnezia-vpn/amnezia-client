import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "../Controls2"
import "../Controls2/TextTypes"

import "../Config"

DrawerType2 {
    id: root
    objectName: "passwordDrawer"

    property bool fromOutside: true
    property string fileName
    property var securedFunc

    expandedStateContent: ColumnLayout {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 16
        anchors.leftMargin: 16
        anchors.rightMargin: 16

        Header2TextType {
            Layout.fillWidth: true
            Layout.bottomMargin: 8

            text: qsTr("Enter password to continue")
        }

        TextFieldWithHeaderType {
            id: passwordField

            Connections {
                target: root

                function onCloseTriggered() {
                    passwordField.textField.text = ""
                }
            }

            property bool hideContent: true

            Layout.fillWidth: true

            headerText: qsTr("Password")
            textField.echoMode: hideContent ? TextInput.Password : TextInput.Normal
            textField.text: ""

            rightButtonClickedOnEnter: true

            clickedFunc: function () {
                hideContent = !hideContent
                buttonImageSource = textField.text !== "" ? (hideContent ? "qrc:/images/controls/eye.svg" : "qrc:/images/controls/eye-off.svg") : ""
            }

            textField.onFocusChanged: {
                textField.text = textField.text.replace(/^\s+|\s+$/g, '')
            }

            textField.onTextChanged: {
                buttonImageSource = textField.text !== "" ? (hideContent ? "qrc:/images/controls/eye.svg" : "qrc:/images/controls/eye-off.svg") : ""
            }
        }

        LabelTextType {
            Layout.fillWidth: true
            Layout.topMargin: 8
            Layout.bottomMargin: 16

            text: fromOutside ? SystemController.readHint(fileName) : SettingsController.getHint()
        }

        BasicButtonType {
            id: doneButton

            Layout.fillWidth: true

            text: qsTr("Done")

            clickedFunc: function() {
                if (fromOutside) {
                    if (!SystemController.decryptFile(fileName, passwordField.textField.text)) {
                        passwordField.errorText = qsTr("Incorrect password")
                        return
                    }
                } else {
                    if (passwordField.textField.text !== SettingsController.getPassword()) {
                        passwordField.errorText = qsTr("Incorrect password")
                        return
                    }
                }

                if (root.securedFunc && typeof root.securedFunc === "function") {
                    root.securedFunc()
                }

                root.closeTriggered()
            }
        }
    }
}
