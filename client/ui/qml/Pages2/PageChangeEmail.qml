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

    defaultActiveFocusItem: newEmailField.textField

    property string newEmail
    property string newEmailConfirmation

    RowLayout {
        id: topStrip

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20

        BackButtonType {
            id: backButton

            KeyNavigation.tab: emailField.textField

            backButtonFunction: function() {
                PageController.closePage()
            }
        }

        HeaderType {
            Layout.fillWidth: true
            Layout.rightMargin: 16
            Layout.leftMargin: 16

            headerText: qsTr("Change email")
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
                headerText: qsTr("Email change")
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.maximumWidth: parent.width

                font.pixelSize: 14

                text: qsTr("A confirmation link has been sent to your old email.")
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
            anchors.right: parent.right
            anchors.left: parent.left
            anchors.leftMargin: 16
            anchors.rightMargin: 16

            spacing: 16

            TextFieldWithHeaderType {
                id: newEmailField

                Layout.fillWidth: true
                headerText: qsTr("New email address")

                textFieldText: root.newEmail

                textField.onTextEdited: function() {
                    if (textField.text !== root.newEmailConfirmation) {
                        emailMatchError.opacity = 1
                        submitButton.enabled = false
                    } else {
                        emailMatchError.opacity = 0
                        submitButton.enabled = true
                    }
                }

                KeyNavigation.tab: newEmailConfirmationField.textField
            }
            Binding { root.newEmail: newEmailField.textField.text }

            TextFieldWithHeaderType {
                id: newEmailConfirmationField

                Layout.fillWidth: true
                headerText: qsTr("Confirm new email address")

                textFieldText: root.newEmailConfirmation

                textField.onTextEdited: function() {
                    if (textField.text !== root.newEmail) {
                        emailMatchError.opacity = 1
                        submitButton.enabled = false
                    } else {
                        emailMatchError.opacity = 0
                        submitButton.enabled = true
                    }
                }

                KeyNavigation.tab: submitButton
            }
            Binding { root.newEmailConfirmation: newEmailConfirmationField.textField.text }

            Header2TextType {
                id: emailMatchError

                color: AmneziaStyle.color.vibrantRed
                font.pixelSize: 14
                font.weight: 500
                opacity: 0

                text: qsTr("Emails do not match")

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
                    AuthController.changeEmail(root.newEmail)
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

        function onEmailChanged() {
            PageController.showBusyIndicator(false)
            blockingPopup.open()
        }
    }
}
