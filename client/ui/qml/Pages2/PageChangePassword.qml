import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Config"
import "../Controls2/TextTypes"
import "../Components"

PageType {
    id: root

    defaultActiveFocusItem: currentPasswordField.textField

    property string currentPassword
    property string newPassword
    property string newPasswordConfirmation

    RowLayout {
        id: topStrip

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20

        BackButtonType {
            id: backButton

            KeyNavigation.tab: currentPasswordField.textField

            backButtonFunction: function() {
                PageController.closePage()
            }
        }

        HeaderType {
            Layout.fillWidth: true
            Layout.rightMargin: 16
            Layout.leftMargin: 16

            headerText: qsTr("Change password")
        }
    }

    Popup {
        id: blockingPopup
        anchors.centerIn: Overlay.overlay
        background: Rectangle {
            radius: 16
            color: Qt.rgba(14/255, 14/255, 17/255, 1.0)
            border.color: "transparent"
        }

        enter: Transition {
            NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 100 }
        }
        exit: Transition {
            NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 100 }
        }

        modal: true
        focus: true
        closePolicy: Popup.NoAutoClose

        padding: 20
        property int margin: 32
        property int maxWidth: 380
        width: Math.min(parent.width - margin, maxWidth)

        ColumnLayout {
            id: popupContent
            width: parent.width
            spacing: 16

            Header2Type {
                headerText: qsTr("Password changed")
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.maximumWidth: parent.width

                font.pixelSize: 14

                text: qsTr("Your password has been changed successfully.")
                color: AmneziaStyle.color.paleGray
            }

            BasicButtonType {
                id: goToLoginButton

                Layout.fillWidth: true

                text: qsTr("Go back")

                clickedFunc: function() {
                    blockingPopup.close()
                    PageController.closePage()
                }
            }
        }
    }

    FlickableType {
        id: fl
        anchors.top: topStrip.bottom
        anchors.topMargin: 20
        anchors.bottom: parent.bottom
        contentHeight: content.height

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 16
            anchors.rightMargin: 16

            spacing: 16

            TextFieldWithHeaderType {
                id: currentPasswordField

                Layout.fillWidth: true
                headerText: qsTr("Current password")

                textFieldText: root.currentPassword
                echoMode: TextInput.Password

                KeyNavigation.tab: newPasswordField.textField
            }
            Binding { root.currentPassword: currentPasswordField.textField.text }

            TextFieldWithHeaderType {
                id: newPasswordField

                Layout.fillWidth: true
                headerText: qsTr("New password")

                textFieldText: root.newPassword
                echoMode: TextInput.Password

                textField.onTextEdited: function() {
                    if (textField.text !== root.newPasswordConfirmation) {
                        passwordMatchError.opacity = 1
                        submitButton.enabled = false
                    } else {
                        passwordMatchError.opacity = 0
                        submitButton.enabled = true
                    }
                }

                KeyNavigation.tab: newPasswordConfirmationField.textField
            }
            Binding { root.newPassword: newPasswordField.textField.text }

            TextFieldWithHeaderType {
                id: newPasswordConfirmationField

                Layout.fillWidth: true
                headerText: qsTr("Confirm password")

                textFieldText: root.newPasswordConfirmation
                echoMode: TextInput.Password

                textField.onTextEdited: function() {
                    if (textField.text !== root.newPassword) {
                        passwordMatchError.opacity = 1
                        submitButton.enabled = false
                    } else {
                        passwordMatchError.opacity = 0
                        submitButton.enabled = true
                    }
                }

                KeyNavigation.tab: submitButton
            }
            Binding { root.newPassword: newPasswordConfirmationField.textField.text }

            Header2TextType {
                id: passwordMatchError

                color: AmneziaStyle.color.vibrantRed
                font.pixelSize: 14
                font.weight: 500
                opacity: 0

                text: qsTr("Passwords do not match")

                Behavior on opacity {
                    PropertyAnimation { duration: 200 }
                }
            }

            BasicButtonType {
                id: submitButton

                Layout.fillWidth: true
                text: qsTr("Submit")

                clickedFunc: function() {
                    PageController.showBusyIndicator(true)
                    AuthController.changePassword(root.currentPassword, root.newPassword)
                }

                Keys.onTabPressed: lastItemTabClicked()
            }
        }
    }

    Connections {
        target: AuthController

        function onErrorOccurred(error) {
            PageController.showBusyIndicator(false)
        }

        function onPasswordChanged() {
            PageController.showBusyIndicator(false)
            blockingPopup.open()
        }
    }
}
