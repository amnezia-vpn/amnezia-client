import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Config"
import "../Controls2/TextTypes"

PageType {
    id: root

    property bool isChangingPassword: false

    Connections {
        target: SettingsController

        function onChangingPassword() {
            root.isChangingPassword = true
        }
    }

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20

        onFocusChanged: {
            if (this.activeFocus) {
                listView.positionViewAtBeginning()
            }
        }
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
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: root.isChangingPassword ? qsTr("Confirm new password") : qsTr("Confirm password")
                descriptionText: root.isChangingPassword ? qsTr("") : qsTr("If you forget your password, you'll have to reset all app settings to reset it."
                                                                         + " Encrypted files will remain encrypted")
            }
        }

        model: 1 // fake model
        spacing: 16

        delegate: ColumnLayout {
            width: listView.width

            TextFieldWithHeaderType {
                id: delegate

                property bool hideContent: true

                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: root.isChangingPassword ? qsTr("Enter new password one more time") : qsTr("Enter password one more time")
                textField.echoMode: hideContent ? TextInput.Password : TextInput.Normal
                textField.placeholderText: ""
                textField.text: textField.text

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
        }

        footer: ColumnLayout {
            width: listView.width

            LabelTextType {
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 24

                text: SettingsController.getTempHint()
            }

            BasicButtonType {
                id: continueButton

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: root.isChangingPassword ? qsTr("Save new password") : qsTr("Turn on encryption")

                clickedFunc: function() {
                    if (!root.isPasswordProperlyFilled()) {
                        return
                    }

                    SettingsController.setPassword(SettingsController.getTempPassword())
                    SettingsController.setHint(SettingsController.getTempHint())

                    SettingsController.setTempPassword("")
                    SettingsController.setTempHint("")

                    PageController.closePage()
                    PageController.goToPage(PageEnum.PageSettings)
                    PageController.goToPage(PageEnum.PageSettingsAppEncryption)
                    SettingsController.toggleFileEncryption(true)
                }
            }
        }
    }

    function isPasswordProperlyFilled() {
        var notMatch = false

        var secretDataItem = listView.itemAtIndex(0).children[0]
        if (secretDataItem.textField.text !== SettingsController.getTempPassword()) {
            secretDataItem.errorText = qsTr("Passwords not match")
            notMatch = true
        }

        return !notMatch
    }
}