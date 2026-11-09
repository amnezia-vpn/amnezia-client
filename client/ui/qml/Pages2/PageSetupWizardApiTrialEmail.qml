import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root
    property string trialEmailErrorMessage: ""

    Connections {
        target: SubscriptionUiController

        function onTrialEmailError(message) {
            root.trialEmailErrorMessage = message
            emailField.errorText = message
        }
    }

    FlickableType {
        id: flick

        anchors.top: parent.top
        anchors.bottom: bottomBar.top

        contentHeight: scrollColumn.implicitHeight + 16

        ColumnLayout {
            id: scrollColumn

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 0

            NavigationBarType {
                Layout.fillWidth: true

                title: qsTr("Create an account")
                description: qsTr("To manage your subscription")
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                spacing: 24

                TextFieldType {
                    id: emailField

                    Layout.fillWidth: true

                    headerText: qsTr("Email")
                    textField.placeholderText: qsTr("Email")
                    textField.inputMethodHints: Qt.ImhEmailCharactersOnly

                    Connections {
                        target: emailField.textField

                        function onTextChanged() {
                            if (root.trialEmailErrorMessage !== "") {
                                root.trialEmailErrorMessage = ""
                                emailField.errorText = ""
                            }
                        }
                    }
                }

                CaptionTextType {
                    Layout.fillWidth: true

                    text: qsTr("We will create an account for your trial subscription and send important subscription updates to this email address")
                }
            }
        }
    }

    BottomBarType {
        id: bottomBar

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        ButtonType {
            id: continueButton

            Layout.fillWidth: true

            text: qsTr("Continue")

            onClicked: {
                root.trialEmailErrorMessage = ""
                emailField.errorText = ""

                var raw = emailField.textField.text.trim()
                if (raw.length === 0 || raw.indexOf("@") < 0) {
                    PageController.showNotificationMessage(qsTr("Enter a valid email address"))
                    return
                }
                PageController.showBusyIndicator(true)
                var ok = SubscriptionUiController.importTrialFromGateway(raw)
                PageController.showBusyIndicator(false)
                if (ok) {
                    PageController.closePage()
                    PageController.closePage()
                }
            }
        }
    }
}
